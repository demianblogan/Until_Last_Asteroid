#include "Shot.h"

#include <cmath>
#include <numbers>
#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "core/World.h"

Shot::Shot(AssetStore& assets, World& world, sf::Texture& texture,
	const GameplayData::ProjectileConfig& config)
	: Entity(assets, world, texture)
	, speed(config.speed)
	, damage(config.damage)
	, knockback(config.knockback)
{
}

void Shot::Update(float deltaTime)
{
	Move(deltaTime);

	const sf::Vector2f position{ GetPosition() };
	const World& world{ GetWorld() };
	if (position.x < 0.f || position.x > world.GetWidth() ||
		position.y < 0.f || position.y > world.GetHeight())
	{
		Destroy();
	}
}

int Shot::GetDamage() const noexcept { return damage; }
float Shot::GetKnockback() const noexcept { return knockback; }

void Shot::SetDirection(const sf::Vector2f& direction) noexcept
{
	SetVelocity(direction * speed);
}

PlayerShot::PlayerShot(AssetStore& assets, World& world,
	const sf::Vector2f& position, float rotationDegrees)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::PlayerShot),
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Player))
{
	SetPosition(position);
	const float angle{ rotationDegrees * std::numbers::pi_v<float> / 180.f
		- std::numbers::pi_v<float> / 2.f };
	SetRotation(sf::degrees(rotationDegrees));
	SetDirection({ std::cos(angle), std::sin(angle) });
	GetWorld().AddSound(Config::Sound::PlayerShot);
}

Entity::Type PlayerShot::GetType() const noexcept { return Type::Projectile_Player; }

bool PlayerShot::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Enemy || other.GetType() == Type::Asteroid)
		&& CheckCollision(other);
}

SaucerShot::SaucerShot(AssetStore& assets, World& world,
	const sf::Vector2f& position, const sf::Vector2f& targetPosition, int currentScore)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::EnemySaucerShot),
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Enemy))
{
	SetPosition(position);
	const sf::Vector2f toTarget{ targetPosition - position };
	const float baseAngle{ std::atan2(toTarget.y, toTarget.x) };
	const float spread{ Random::Float(-1.f, 1.f) * std::numbers::pi_v<float>
		/ ((200.f + currentScore) / 100.f) };
	const float finalAngle{ baseAngle + spread };
	SetDirection({ std::cos(finalAngle), std::sin(finalAngle) });
	SetRotation(sf::degrees(finalAngle * 180.f / std::numbers::pi_v<float> + 90.f));
	GetWorld().AddSound(Config::Sound::EnemyShot);
}

Entity::Type SaucerShot::GetType() const noexcept { return Type::Projectile_Enemy; }

bool SaucerShot::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Player || other.GetType() == Type::Asteroid)
		&& CheckCollision(other);
}
