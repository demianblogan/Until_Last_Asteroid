#include "Enemy.h"

#include <cmath>
#include <numbers>
#include "utils/Random.h"

Enemy::Enemy(AssetStore& assets, World& world, sf::Texture& texture, int scoreValue, float speed)
	: Entity(assets, world, texture), scoreValue(scoreValue)
{
	constexpr float TwoPi{ 2.f * std::numbers::pi_v<float> };
	const float angle{ Random::Float(0.f, TwoPi) };
	const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };

	SetVelocity(direction * speed);
}

int Enemy::GetScoreValue() const noexcept
{
	return scoreValue;
}

Entity::Type Enemy::GetType() const noexcept
{
	return Type::Enemy;
}

void Enemy::Update(float deltaTime)
{
	Move(deltaTime);
}