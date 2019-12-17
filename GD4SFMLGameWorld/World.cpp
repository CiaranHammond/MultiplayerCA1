#include "World.hpp"
#include "ParticleID.hpp"
#include "ParticleNode.hpp"
#include <iostream>

#include <SFML/Graphics/RenderWindow.hpp>



World::World(sf::RenderTarget& outputTarget, FontHolder& fonts, SoundPlayer& sounds)
	: mTarget(outputTarget)
	, mSceneTexture()
	, mCamera(outputTarget.getDefaultView())
	, mFonts(fonts)
	, mSounds(sounds)
	, mTextures()
	, mSceneGraph()
	, mSceneLayers()
	, mWorldBounds(0.f, 0.f, mCamera.getSize().x, 5000.f)
	, mSpawnPosition(mCamera.getSize().x / 2.f, mWorldBounds.height - mCamera.getSize().y / 2.f)
	, mScrollSpeed(-50.f)
	, mPlayerAircraft(nullptr)
	, mPlayer2Aircraft(nullptr)
	, mEnemySpawnPoints()
	, mActiveEnemies()
	, mActivePlayers()
{
	mSceneTexture.create(mTarget.getSize().x, mTarget.getSize().y);
	loadTextures();
	buildScene();

	// Prepare the view
	mCamera.setCenter(mSpawnPosition);
}

void World::update(sf::Time dt)
{
	// Scroll the world, reset player velocity
	mCamera.move(0.f, mScrollSpeed * dt.asSeconds());
	mPlayerAircraft->setVelocity(0.f, 0.f);
	mPlayer2Aircraft->setVelocity(0.f, 0.f);

	// Setup commands to destroy entities, and guide missiles
	destroyEntitiesOutsideView();
	guideMissiles();

	// Forward commands to scene graph, adapt velocity (scrolling, diagonal correction)
	while (!mCommandQueue.isEmpty())
		mSceneGraph.onCommand(mCommandQueue.pop(), dt);
	adaptPlayerVelocity();
	adaptPlayer2Velocity();

	// Collision detection and response (may destroy entities)
	handleCollisions();

	// Remove all destroyed entities, create new ones
	mSceneGraph.removeWrecks();
	spawnEnemies();

	// Regular update step, adapt position (correct if outside view)
	mSceneGraph.update(dt, mCommandQueue);
	adaptPlayerPosition();
	adaptPlayer2Position();

	updateSounds();
}

void World::draw()
{
	if (PostEffect::isSupported())
	{
		mSceneTexture.clear();
		mSceneTexture.setView(mCamera);
		mSceneTexture.draw(mSceneGraph);
		mSceneTexture.display();
		mBloomEffect.apply(mSceneTexture, mTarget);
	}
	else
	{
		mTarget.setView(mCamera);
		mTarget.draw(mSceneGraph);
	}
}

CommandQueue& World::getCommandQueue()
{
	return mCommandQueue;
}

bool World::hasAlivePlayer() const
{
	return !mPlayerAircraft->isMarkedForRemoval() && !mPlayer2Aircraft->isMarkedForRemoval();
}

bool World::hasPlayerReachedEnd() const
{
	return !mWorldBounds.contains(mPlayerAircraft->getPosition()) && !mWorldBounds.contains(mPlayer2Aircraft->getPosition());
}

void World::updateSounds()
{
	//Set the listener to the player position
	mSounds.setListenPosition(mPlayerAircraft->getWorldPosition());
	mSounds.setListenPosition(mPlayer2Aircraft->getWorldPosition());
	//Remove unused sounds
	mSounds.removeStoppedSounds();

}

void World::loadTextures()
{
	mTextures.load(TextureID::Entities, "Media/Textures/Entities.png");
	mTextures.load(TextureID::Jungle, "Media/Textures/Jungle.png");
	mTextures.load(TextureID::Explosion, "Media/Textures/Explosion.png");
	mTextures.load(TextureID::Particle, "Media/Textures/Particle.png");
	mTextures.load(TextureID::FinishLine, "Media/Textures/FinishLine.png");
}

bool matchesCategories(SceneNode::Pair& colliders, CategoryID type1, CategoryID type2)
{
	unsigned int category1 = colliders.first->getCategory();
	unsigned int category2 = colliders.second->getCategory();

	// Make sure first pair entry has category type1 and second has type2
	if (((static_cast<int>(type1))& category1) && ((static_cast<int>(type2))& category2))
	{
		return true;
	}
	else if (((static_cast<int>(type1))& category2) && ((static_cast<int>(type2))& category1))
	{
		std::swap(colliders.first, colliders.second);
		return true;
	}
	else
	{
		return false;
	}
}

void World::handleCollisions()
{
	std::set<SceneNode::Pair> collisionPairs;
	mSceneGraph.checkSceneCollision(mSceneGraph, collisionPairs);

	for (SceneNode::Pair pair : collisionPairs)
	{
		if (matchesCategories(pair, CategoryID::PlayerAircraft, CategoryID::EnemyAircraft))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& enemy = static_cast<Aircraft&>(*pair.second);

			// Collision: Player damage = enemy's remaining HP
			player.damage(enemy.getHitpoints());
			enemy.destroy();
		}

		else if (matchesCategories(pair, CategoryID::Player2Aircraft, CategoryID::EnemyAircraft))
		{
			auto& player2 = static_cast<Aircraft&>(*pair.first);
			auto& enemy = static_cast<Aircraft&>(*pair.second);

			//Collision: Player damage = enemy's remaining HP
			player2.damage(enemy.getHitpoints());
			enemy.destroy();
		}

		else if (matchesCategories(pair, CategoryID::PlayerAircraft, CategoryID::Pickup))
		{
			auto& player = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			// Apply pickup effect to player, destroy projectile
			pickup.apply(player);
			player.playerLocalSound(mCommandQueue, SoundEffectID::CollectPickup);
			pickup.destroy();
		}

		else if (matchesCategories(pair, CategoryID::Player2Aircraft, CategoryID::Pickup))
		{
			auto& player2 = static_cast<Aircraft&>(*pair.first);
			auto& pickup = static_cast<Pickup&>(*pair.second);

			// Apply pickup effect to player, destroy projectile
			pickup.apply(player2);
			player2.playerLocalSound(mCommandQueue, SoundEffectID::CollectPickup);
			pickup.destroy();
		}

		else if (matchesCategories(pair, CategoryID::EnemyAircraft, CategoryID::AlliedProjectile)
			|| matchesCategories(pair, CategoryID::PlayerAircraft, CategoryID::EnemyProjectile)
			|| matchesCategories(pair, CategoryID::Player2Aircraft, CategoryID::EnemyProjectile))
		{
			auto& aircraft = static_cast<Aircraft&>(*pair.first);
			auto& projectile = static_cast<Projectile&>(*pair.second);

			// Apply projectile damage to aircraft, destroy projectile
			aircraft.damage(projectile.getDamage());
			projectile.destroy();
		}
	}
}

void World::buildScene()
{
	// Initialize the different layers
	for (std::size_t i = 0; i < static_cast<int>(LayerID::LayerCount); ++i)
	{
		CategoryID category = (i == (static_cast<int>(LayerID::LowerAir))) ? CategoryID::SceneAirLayer : CategoryID::None;

		SceneNode::Ptr layer(new SceneNode(category));
		mSceneLayers[i] = layer.get();

		mSceneGraph.attachChild(std::move(layer));
	}

	// Prepare the tiled background

	sf::Texture& texture = mTextures.get(TextureID::Jungle);
	sf::IntRect textureRect(mWorldBounds);
	texture.setRepeated(true);

	// Add the background sprite to the scene
	std::unique_ptr<SpriteNode> backgroundSprite(new SpriteNode(texture, textureRect));
	backgroundSprite->setPosition(mWorldBounds.left, mWorldBounds.top);
	mSceneLayers[static_cast<int>(LayerID::Background)]->attachChild(std::move(backgroundSprite));

	//Add the finish line to the scene
	sf::Texture& finishTexture = mTextures.get(TextureID::FinishLine);
	std::unique_ptr<SpriteNode> finishSprite(new SpriteNode(finishTexture));
	finishSprite->setPosition(0.f, -76.f);
	mSceneLayers[static_cast<int>(LayerID::Background)]->attachChild(std::move(finishSprite));

	//Add particle nodes for smoke and propellant
	std::unique_ptr<ParticleNode> smokeNode(new ParticleNode(ParticleID::Smoke, mTextures));
	mSceneLayers[static_cast<int>(LayerID::LowerAir)]->attachChild(std::move(smokeNode));

	std::unique_ptr<ParticleNode> propellantNode(new ParticleNode(ParticleID::Propellant, mTextures));
	mSceneLayers[static_cast<int>(LayerID::LowerAir)]->attachChild(std::move(propellantNode));

	//Add the sound effect node
	std::unique_ptr<SoundNode> soundNode(new SoundNode(mSounds));
	mSceneGraph.attachChild(std::move(soundNode));

	// Add player's aircraft
	std::unique_ptr<Aircraft> player1(new Aircraft(AircraftID::Player, mTextures, mFonts));
	mPlayerAircraft = player1.get();
	mPlayerAircraft->setPosition(mSpawnPosition);
	mSceneLayers[static_cast<int>(LayerID::UpperAir)]->attachChild(std::move(player1));

	std::unique_ptr<Aircraft> player2(new Aircraft(AircraftID::Player2, mTextures, mFonts));
	mPlayer2Aircraft = player2.get();
	mPlayer2Aircraft->setPosition(mSpawnPosition.x + 100, mSpawnPosition.y);
	mSceneLayers[static_cast<int>(LayerID::UpperAir)]->attachChild(std::move(player2));

	addEnemies();
}

void World::adaptPlayerPosition()
{
	// Keep player's position inside the screen bounds, at least borderDistance units from the border
	sf::FloatRect viewBounds = getViewBounds();
	const float borderDistance = 40.f;

	sf::Vector2f position = mPlayerAircraft->getPosition();
	position.x = std::max(position.x, viewBounds.left + borderDistance);
	position.x = std::min(position.x, viewBounds.left + viewBounds.width - borderDistance);
	position.y = std::max(position.y, viewBounds.top + borderDistance);
	position.y = std::min(position.y, viewBounds.top + viewBounds.height - borderDistance);
	mPlayerAircraft->setPosition(position);

	sf::Vector2f position2 = mPlayer2Aircraft->getPosition();
	position2.x = std::max(position2.x, viewBounds.left + borderDistance);
	position2.x = std::min(position2.x, viewBounds.left + viewBounds.width - borderDistance);
	position2.y = std::max(position2.y, viewBounds.top + borderDistance);
	position2.y = std::min(position2.y, viewBounds.top + viewBounds.height - borderDistance);
	mPlayer2Aircraft->setPosition(position2);
}

void World::adaptPlayerVelocity()
{
	sf::Vector2f velocity = mPlayerAircraft->getVelocity();
	

	// If moving diagonally, reduce velocity (to have always same velocity)
	if (velocity.x != 0.f && velocity.y != 0.f)
		mPlayerAircraft->setVelocity(velocity / std::sqrt(2.f));

	

	// Add scrolling velocity
	mPlayerAircraft->accelerate(0.f, mScrollSpeed);
}

void World::adaptPlayer2Position()
{
	// Keep player's position inside the screen bounds, at least borderDistance units from the border
	sf::FloatRect viewBounds = getViewBounds();
	const float borderDistance = 40.f;


	sf::Vector2f position2 = mPlayer2Aircraft->getPosition();
	position2.x = std::max(position2.x, viewBounds.left + borderDistance);
	position2.x = std::min(position2.x, viewBounds.left + viewBounds.width - borderDistance);
	position2.y = std::max(position2.y, viewBounds.top + borderDistance);
	position2.y = std::min(position2.y, viewBounds.top + viewBounds.height - borderDistance);
	mPlayer2Aircraft->setPosition(position2);
}

void World::adaptPlayer2Velocity()
{
	sf::Vector2f velocity2 = mPlayer2Aircraft->getVelocity();

	// If moving diagonally, reduce velocity (to have always same velocity)

	if (velocity2.x != 0.f && velocity2.y != 0.f)
		mPlayer2Aircraft->setVelocity(velocity2 / std::sqrt(2.f));

	// Add scrolling velocity
	mPlayer2Aircraft->accelerate(0.f, mScrollSpeed);
}

void World::addEnemies()
{
	// Add enemies to the spawn point container
	addEnemy(AircraftID::Raptor, 0.f, 450.f);
	addEnemy(AircraftID::Raptor, +100.f, 500.f);
	addEnemy(AircraftID::Raptor, +100.f, 600.f);
	addEnemy(AircraftID::Raptor, -100.f, 650.f);
	addEnemy(AircraftID::Raptor, 70.f, 650.f);
	addEnemy(AircraftID::Raptor, -70.f, 650.f);

	addEnemy(AircraftID::Raptor, -70.f, 700.f);
	addEnemy(AircraftID::Raptor, 70.f, 700.f);
	addEnemy(AircraftID::Avenger, 30.f, 800.f);
	addEnemy(AircraftID::Raptor, 300.f, 800.f);
	addEnemy(AircraftID::Raptor, -300.f, 900.f);
	addEnemy(AircraftID::Raptor, 0.f, 900.f);
	addEnemy(AircraftID::Raptor, 0.f, 1000.f);
	addEnemy(AircraftID::Avenger, -300.f, 1000.f);
	addEnemy(AircraftID::Avenger, -300.f, 1100.f);
	addEnemy(AircraftID::Raptor, 0.f, 1100.f);
	addEnemy(AircraftID::Raptor, 250.f, 1200.f);
	addEnemy(AircraftID::Raptor, -250.f, 1200.f);
	addEnemy(AircraftID::Raptor, 0.f, 1300.f);
	addEnemy(AircraftID::Avenger, 0.f, 1300.f);
	addEnemy(AircraftID::Raptor, 0.f, 1400.f);
	addEnemy(AircraftID::Avenger, 0.f, 1400.f);
	addEnemy(AircraftID::Avenger, -200.f, 1500.f);
	addEnemy(AircraftID::Raptor, 200.f, 1600.f);
	addEnemy(AircraftID::Raptor, 0.f, 1650.f);

	addEnemy(AircraftID::Raptor, -70.f, 1650.f);
	addEnemy(AircraftID::Raptor, 70.f, 1700.f);
	addEnemy(AircraftID::Avenger, 30.f, 1700.f);
	addEnemy(AircraftID::Raptor, 300.f, 1750.f);
	addEnemy(AircraftID::Raptor, -300.f, 1750.f);
	addEnemy(AircraftID::Raptor, 0.f, 1800.f);
	addEnemy(AircraftID::Raptor, 0.f, 1800.f);
	addEnemy(AircraftID::Avenger, -300.f, 1900.f);
	addEnemy(AircraftID::Avenger, -300.f, 1900.f);
	addEnemy(AircraftID::Raptor, 0.f, 1950.f);
	addEnemy(AircraftID::Raptor, 250.f, 1950.f);
	addEnemy(AircraftID::Raptor, -250.f, 2000.f);
	addEnemy(AircraftID::Raptor, 0.f, 2500.f);
	addEnemy(AircraftID::Avenger, 0.f, 2500.f);
	addEnemy(AircraftID::Raptor, 0.f, 2550.f);
	addEnemy(AircraftID::Avenger, 0.f, 2550.f);
	addEnemy(AircraftID::Avenger, -200.f, 3000.f);
	addEnemy(AircraftID::Raptor, 200.f, 3000.f);
	addEnemy(AircraftID::Raptor, 0.f, 3050.f);

	addEnemy(AircraftID::Raptor, -70.f, 3050.f);
	addEnemy(AircraftID::Raptor, 70.f, 4000.f);
	addEnemy(AircraftID::Avenger, 30.f, 4000.f);
	addEnemy(AircraftID::Raptor, 300.f, 4050.f);
	addEnemy(AircraftID::Raptor, -300.f, 4050.f);
	addEnemy(AircraftID::Raptor, 0.f, 5000.f);
	addEnemy(AircraftID::Raptor, 0.f, 5000.f);
	addEnemy(AircraftID::Avenger, -300.f, 6000.f);
	addEnemy(AircraftID::Avenger, -300.f, 6000.f);
	addEnemy(AircraftID::Raptor, 0.f, 7050.f);
	addEnemy(AircraftID::Raptor, 250.f, 7050.f);
	addEnemy(AircraftID::Raptor, -250.f, 8000.f);
	addEnemy(AircraftID::Raptor, 0.f, 8500.f);
	addEnemy(AircraftID::Avenger, 0.f, 8500.f);
	addEnemy(AircraftID::Raptor, 0.f, 9000.f);
	addEnemy(AircraftID::Avenger, 0.f, 9000.f);
	addEnemy(AircraftID::Avenger, -200.f, 9050.f);
	addEnemy(AircraftID::Raptor, 200.f, 9050.f);
	addEnemy(AircraftID::Raptor, 0.f, 10000.f);

	// Sort all enemies according to their y value, such that lower enemies are checked first for spawning
	std::sort(mEnemySpawnPoints.begin(), mEnemySpawnPoints.end(), [](SpawnPoint lhs, SpawnPoint rhs)
	{
		return lhs.y < rhs.y;
	});
}

void World::addEnemy(AircraftID type, float relX, float relY)
{
	SpawnPoint spawn(type, mSpawnPosition.x + relX, mSpawnPosition.y - relY);
	mEnemySpawnPoints.push_back(spawn);
}

void World::spawnEnemies()
{
	// Spawn all enemies entering the view area (including distance) this frame
	while (!mEnemySpawnPoints.empty()
		&& mEnemySpawnPoints.back().y > getBattlefieldBounds().top)
	{
		SpawnPoint spawn = mEnemySpawnPoints.back();

		std::unique_ptr<Aircraft> enemy(new Aircraft(spawn.type, mTextures, mFonts));
		enemy->setPosition(spawn.x, spawn.y);
		enemy->setRotation(180.f);

		mSceneLayers[static_cast<int>(LayerID::UpperAir)]->attachChild(std::move(enemy));

		// Enemy is spawned, remove from the list to spawn
		mEnemySpawnPoints.pop_back();
	}

	/*Command playerCollector;
	playerCollector.category = static_cast<int>(CategoryID::PlayerAircraft);
	playerCollector.action = derivedAction<Aircraft>([this](Aircraft& player, sf::Time)
		{
			if (!player.isDestroyed())
				mActivePlayers.push_back(&player);
		});

	Command zombieGuider;
	zombieGuider.category = static_cast<int>(CategoryID::EnemyAircraft);
	zombieGuider.action = derivedAction<Aircraft>([this](Aircraft& zombie, sf::Time)
		{
			if (!zombie.isGuided())
				return;

			float minDistance = std::numeric_limits<float>::max();
			Aircraft* closestPlayer = nullptr;

			for (Aircraft* player : mActivePlayers)
			{
				float playerDistance = distance(zombie, *player);

				if (playerDistance < minDistance)
				{
					closestPlayer = player;
					minDistance = playerDistance;
				}
			}

			if (closestPlayer)
				zombie.guideTowards(closestPlayer->getWorldPosition());
		});

	mCommandQueue.push(playerCollector);
	mCommandQueue.push(zombieGuider);
	mActivePlayers.clear();*/
}

void World::destroyEntitiesOutsideView()
{
	Command command;
	command.category = static_cast<int>(CategoryID::Projectile) | static_cast<int>(CategoryID::EnemyAircraft);
	command.action = derivedAction<Entity>([this](Entity& e, sf::Time)
	{
		if (!getBattlefieldBounds().intersects(e.getBoundingRect()))
			e.destroy();
	});

	mCommandQueue.push(command);
}

void World::guideMissiles()
{
	// Setup command that stores all enemies in mActiveEnemies
	Command enemyCollector;
	enemyCollector.category = static_cast<int>(CategoryID::EnemyAircraft);
	enemyCollector.action = derivedAction<Aircraft>([this](Aircraft& enemy, sf::Time)
	{
		if (!enemy.isDestroyed())
			mActiveEnemies.push_back(&enemy);
	});

	

	// Setup command that guides all missiles to the enemy which is currently closest to the player
	Command missileGuider;
	missileGuider.category = static_cast<int>(CategoryID::AlliedProjectile);
	missileGuider.action = derivedAction<Projectile>([this](Projectile& missile, sf::Time)
	{
		// Ignore unguided bullets
		if (!missile.isGuided())
			return;

		float minDistance = std::numeric_limits<float>::max();
		Aircraft* closestEnemy = nullptr;

		// Find closest enemy
		for (Aircraft* enemy : mActiveEnemies)
		{
			float enemyDistance = distance(missile, *enemy);

			if (enemyDistance < minDistance)
			{
				closestEnemy = enemy;
				minDistance = enemyDistance;
			}
		}

		if (closestEnemy)
			missile.guideTowards(closestEnemy->getWorldPosition());
	});

	// Push commands, reset active enemies
	mCommandQueue.push(enemyCollector);
	mCommandQueue.push(missileGuider);
	mActiveEnemies.clear();
}

sf::FloatRect World::getViewBounds() const
{
	return sf::FloatRect(mCamera.getCenter() - mCamera.getSize() / 2.f, mCamera.getSize());
}

sf::FloatRect World::getBattlefieldBounds() const
{
	// Return view bounds + some area at top, where enemies spawn
	sf::FloatRect bounds = getViewBounds();
	bounds.top -= 100.f;
	bounds.height += 100.f;

	return bounds;
}