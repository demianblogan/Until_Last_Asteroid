#include "Shot.h"

#include <cmath>
#include <numbers>
#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "core/World.h"

Shot::Shot(AssetStore& assets, World& world, sf::Texture& texture, float speed) noexcept
	: Entity(assets, world, texture)
	, speed(speed)
{
	// No code
}

void Shot::Update(float deltaTime)
{
	Move(deltaTime);

	sf::Vector2f position{ GetPosition() };
	const World& world{ GetWorld() };

	if (position.x < 0.f || position.x > world.GetWidth() ||
		position.y < 0.f || position.y > world.GetHeight())
	{
		Destroy();
	}
}

void Shot::SetDirection(const sf::Vector2f& direction) noexcept
{
	SetVelocity(direction * speed);
}

PlayerShot::PlayerShot(AssetStore& assets, World& world, const sf::Vector2f& position, float rotationDegrees)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::PlayerShot), SPEED)
{
	SetPosition(position);

	const float angleInRadians{ rotationDegrees * std::numbers::pi_v<float> / 180.f - std::numbers::pi_v<float> / 2.f };

	sf::Vector2f direction
	{
		std::cos(angleInRadians),
		std::sin(angleInRadians)
	};

	SetRotation(sf::degrees(rotationDegrees));
	SetDirection(direction);

	GetWorld().AddSound(Config::Sound::PlayerLaserShot);
}

Entity::Type PlayerShot::GetType() const noexcept
{
	return Type::Projectile_Player;
}

bool PlayerShot::IsCollideWith(const Entity& other) const
{
	if (other.GetType() != Type::Enemy && other.GetType() != Type::Asteroid)
		return false;

	return CheckCollision(other);
}

SaucerShot::SaucerShot(AssetStore& assets, World& world, const sf::Vector2f& position, const sf::Vector2f& targetPosition, int currentScore)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::EnemySaucerShot), SPEED)
{
	SetPosition(position);

	sf::Vector2f toTarget{ targetPosition - position };
	float baseAngle{ std::atan2(toTarget.y, toTarget.x) };
	float spread{ Random::Float(-1.f, 1.f) * std::numbers::pi_v<float> / ((200.f + currentScore) / 100.f) };
	float finalAngle{ baseAngle + spread };
	sf::Vector2f direction{ std::cos(finalAngle), std::sin(finalAngle) };

	SetDirection(direction);

	// Sprite faces up, adjust angle from math coordinate system
	float spriteRotationOffset{ 90.f };
	float degrees{ finalAngle * 180.f / std::numbers::pi_v<float> +spriteRotationOffset };

	SetRotation(sf::degrees(degrees));
	GetWorld().AddSound(Config::Sound::EnemyLaserShot);
}

Entity::Type SaucerShot::GetType() const noexcept
{
	return Type::Projectile_Enemy;
}

bool SaucerShot::IsCollideWith(const Entity& other) const
{
	if (other.GetType() != Type::Player && other.GetType() != Type::Asteroid)
		return false;

	return CheckCollision(other);
}