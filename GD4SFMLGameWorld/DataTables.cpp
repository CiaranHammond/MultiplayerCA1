#include "DataTables.hpp"
#include "Aircraft.hpp"
#include "Projectile.hpp"
#include "Pickup.hpp"
#include "PersonID.hpp"
#include "ProjectileID.hpp"
#include "PickupID.hpp"
#include "ParticleID.hpp"





std::vector<AircraftData> initializeAircraftData()
{
	std::vector<AircraftData> data(static_cast<int>(PersonID::TypeCount));
	data[static_cast<int>(PersonID::Player)].hitpoints = 100;
	data[static_cast<int>(PersonID::Player)].speed = 200.f;
	data[static_cast<int>(PersonID::Player)].fireInterval = sf::seconds(1);
	data[static_cast<int>(PersonID::Player)].textureRect = sf::IntRect(0, 0, 48, 100);
	data[static_cast<int>(PersonID::Player)].texture = TextureID::Entities;
	data[static_cast<int>(PersonID::Player)].hasRollAnimation = true;

	data[static_cast<int>(PersonID::Player2)].hitpoints = 100;
	data[static_cast<int>(PersonID::Player2)].speed = 200.f;
	data[static_cast<int>(PersonID::Player2)].fireInterval = sf::seconds(1);
	data[static_cast<int>(PersonID::Player2)].textureRect = sf::IntRect(0, 0, 48, 100);
	data[static_cast<int>(PersonID::Player2)].texture = TextureID::Entities;
	data[static_cast<int>(PersonID::Player2)].hasRollAnimation = true;

	data[static_cast<int>(PersonID::Zombie)].hitpoints = 20;
	data[static_cast<int>(PersonID::Zombie)].speed = 80.f;
	data[static_cast<int>(PersonID::Zombie)].fireInterval = sf::Time::Zero;
	data[static_cast<int>(PersonID::Zombie)].texture = TextureID::Entities;
	data[static_cast<int>(PersonID::Zombie)].textureRect = sf::IntRect(228, 0, 50, 100);

	data[static_cast<int>(PersonID::Zombie)].directions.push_back(Direction(+45.f, 80.f));
	data[static_cast<int>(PersonID::Zombie)].directions.push_back(Direction(-45.f, 160.f));
	data[static_cast<int>(PersonID::Zombie)].directions.push_back(Direction(+45.f, 80.f));
	data[static_cast<int>(PersonID::Zombie)].hasRollAnimation = false;

	data[static_cast<int>(PersonID::SpecialZombie)].hitpoints = 40;
	data[static_cast<int>(PersonID::SpecialZombie)].speed = 50.f;
	data[static_cast<int>(PersonID::SpecialZombie)].fireInterval = sf::seconds(2);
	data[static_cast<int>(PersonID::SpecialZombie)].texture = TextureID::Entities;
	data[static_cast<int>(PersonID::SpecialZombie)].textureRect = sf::IntRect(150, 0, 70, 100);
	data[static_cast<int>(PersonID::SpecialZombie)].directions.push_back(Direction(+45.f, 50.f));
	data[static_cast<int>(PersonID::SpecialZombie)].directions.push_back(Direction(0.f, 50.f));
	data[static_cast<int>(PersonID::SpecialZombie)].directions.push_back(Direction(-45.f, 100.f));
	data[static_cast<int>(PersonID::SpecialZombie)].directions.push_back(Direction(0.f, 50.f));
	data[static_cast<int>(PersonID::SpecialZombie)].directions.push_back(Direction(+45.f, 50.f));
	data[static_cast<int>(PersonID::SpecialZombie)].hasRollAnimation = false;
	data[static_cast<int>(PersonID::SpecialZombie)].hasRollAnimation = false;

	return data;
}

std::vector<ProjectileData> initializeProjectileData()
{
	std::vector<ProjectileData> data(static_cast<int>(ProjectileID::TypeCount));

	data[static_cast<int>(ProjectileID::AlliedBullet)].damage = 10;
	data[static_cast<int>(ProjectileID::AlliedBullet)].speed = 300.f;
	data[static_cast<int>(ProjectileID::AlliedBullet)].texture = TextureID::Entities;
	data[static_cast<int>(ProjectileID::AlliedBullet)].textureRect = sf::IntRect(175, 64, 3, 14);

	data[static_cast<int>(ProjectileID::EnemyBullet)].damage = 10;
	data[static_cast<int>(ProjectileID::EnemyBullet)].speed = 300.f;
	data[static_cast<int>(ProjectileID::EnemyBullet)].texture = TextureID::Entities;
	data[static_cast<int>(ProjectileID::EnemyBullet)].textureRect = sf::IntRect(175, 64, 3, 14);


	data[static_cast<int>(ProjectileID::Missile)].damage = 200;
	data[static_cast<int>(ProjectileID::Missile)].speed = 250.f;
	data[static_cast<int>(ProjectileID::Missile)].texture = TextureID::Entities;
	data[static_cast<int>(ProjectileID::Missile)].textureRect = sf::IntRect(160, 64, 15, 32);

	return data;
}

std::vector<PickupData> initializePickupData()
{
	std::vector<PickupData> data(static_cast<int>(PickupID::TypeCount));
	data[static_cast<int>(PickupID::HealthRefill)].texture = TextureID::Entities;
	data[static_cast<int>(PickupID::HealthRefill)].textureRect = sf::IntRect(0, 64, 40, 40);
	data[static_cast<int>(PickupID::HealthRefill)].action = [](Aircraft& a) {a.repair(25); };

	data[static_cast<int>(PickupID::MissileRefill)].texture = TextureID::Entities;
	data[static_cast<int>(PickupID::MissileRefill)].textureRect = sf::IntRect(40, 64, 40, 40);
	data[static_cast<int>(PickupID::MissileRefill)].action = std::bind(&Aircraft::collectMissiles, std::placeholders::_1, 3);

	data[static_cast<int>(PickupID::FireSpread)].texture = TextureID::Entities;
	data[static_cast<int>(PickupID::FireSpread)].textureRect = sf::IntRect(80, 64, 40, 40);
	data[static_cast<int>(PickupID::FireSpread)].action = std::bind(&Aircraft::increaseSpread, std::placeholders::_1);

	data[static_cast<int>(PickupID::FireRate)].texture = TextureID::Entities;
	data[static_cast<int>(PickupID::FireRate)].textureRect = sf::IntRect(120, 64, 40, 40);
	data[static_cast<int>(PickupID::FireRate)].action = std::bind(&Aircraft::increaseFireRate, std::placeholders::_1);

	return data;
}

std::vector<ParticleData> initializeParticleData()
{
	std::vector<ParticleData> data(static_cast<int>(ParticleID::ParticleCount));

	data[static_cast<int>(ParticleID::Propellant)].color = sf::Color(255, 255, 50);
	data[static_cast<int>(ParticleID::Propellant)].lifetime = sf::seconds(0.6f);

	data[static_cast<int>(ParticleID::Smoke)].color = sf::Color(50, 50, 50);
	data[static_cast<int>(ParticleID::Smoke)].lifetime = sf::seconds(4.f);

	return data;
}
