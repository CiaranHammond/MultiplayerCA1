#pragma once

//Entity/SceneNode category, used to dispatch commands
enum class CategoryID
{
	None = 0,
	SceneAirLayer = 1 << 0,
	PlayerAircraft = 1 << 1,
	Player2Aircraft = 1 << 2,
	EnemyAircraft = 1 << 3,
	Pickup = 1 << 4,
	AlliedProjectile = 1 << 5,
	EnemyProjectile = 1 << 6,
	ParticleSystem = 1 << 7,
	SoundEffect = 1 << 8,

	Aircraft = PlayerAircraft | Player2Aircraft | EnemyAircraft,
	Projectile = AlliedProjectile | EnemyProjectile,
};