#pragma once
#include "Entity.hpp"
#include "PersonID.hpp"
#include "ResourceIdentifiers.hpp"
#include "CommandQueue.hpp"
#include "TextNode.hpp"
#include "Projectile.hpp"
#include "Animation.hpp"

class Aircraft : public Entity
{
public:
	Aircraft(PersonID type, const TextureHolder& textures, const FontHolder& fonts);
	virtual unsigned int getCategory() const;
	virtual sf::FloatRect getBoundingRect() const;
	virtual bool isMarkedForRemoval() const;

	float getMaxSpeed() const;
	void fire();
	void launchMissile();
	bool isAllied() const;
	bool isAllied2() const;
	void increaseFireRate();
	void increaseSpread();
	void collectMissiles(unsigned int count);
	bool isGuided() const;
	void guideTowards(sf::Vector2f position);

	void playerLocalSound(CommandQueue& command, SoundEffectID effect);

private:
	virtual void drawCurrent(sf::RenderTarget& target, sf::RenderStates states) const;
	virtual void updateCurrent(sf::Time dt, CommandQueue& commands);
	void updateMovementPattern(sf::Time dt);
	void updateTexts();

	void checkProjectileLaunch(sf::Time dt, CommandQueue& commands);

	void createBullets(SceneNode& node, const TextureHolder& textures) const;
	void createProjectile(SceneNode& node, ProjectileID type, float xOffset, float yOffset, const TextureHolder& textures) const;

	void createPickup(SceneNode& node, const TextureHolder& textures) const;
	void checkPickupDrop(CommandQueue& commands);
	void updateRollAnimation();

private:
	PersonID mType;
	sf::Sprite mSprite;
	Animation mBloodSplat;
	TextNode* mHealthDisplay;
	TextNode* mMissileDisplay;

	bool mIsFiring;
	bool mIsLaunchingMissile;
	int	mFireRateLevel;
	sf::Time mFireCountdown;

	bool mIsMarkedForRemoval;

	Command mFireCommand;
	Command	mMissileCommand;
	Command mDropPickupCommand;
	bool mShowBloodSplat;
	bool mPlayedBloodSplatSound;
	bool mSpawnedPickup;
	int mSpreadLevel;

	int mMissileAmmo;
	float mTravelledDistance;
	std::size_t mDirectionIndex;
	sf::Vector2f mTargetDirection;
};