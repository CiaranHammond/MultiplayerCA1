#include "Entity.hpp"

#include <cassert>
#include <iostream>

Entity::Entity(int hitpoints)
	: mVelocity(), mHitpoints(hitpoints)
{}

void Entity::setVelocity(sf::Vector2f velocity)
{
	mVelocity = velocity;
}

void Entity::setVelocity(float vx, float vy)
{
	mVelocity.x = vx;
	mVelocity.y = vy;
}

void Entity::accelerate(sf::Vector2f velocity)
{
	mVelocity += velocity;
}

void Entity::accelerate(float vx, float vy)
{
	mVelocity.x += vx;
	mVelocity.y += vy;
}

sf::Vector2f Entity::getVelocity() const
{
	return mVelocity;
}

int Entity::getHitpoints() const
{
	return mHitpoints;
}

void Entity::repair(int points)
{
	assert(points > 0);
	mHitpoints += points;

}

//the points being put in are 0 so it was breaking
void Entity::damage(int points)
{
	std::cout << points << std::endl;
	assert(points >= 0);
	mHitpoints -= points;
}

void Entity::destroy()
{
	mHitpoints = 0;
}

bool Entity::isDestroyed() const
{
	return mHitpoints <= 0;
}

void Entity::updateCurrent(sf::Time dt, CommandQueue&)
{
	move(mVelocity * dt.asSeconds());
}
