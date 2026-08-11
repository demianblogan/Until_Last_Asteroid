#include "Shot.h"

#include <cmath>
#include <numbers>
#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"
#include "core/World.h"

Shot::Shot(AssetStore& assets, World& world, sf::Texture& texture,
	const GameplayData::ProjectileConfig& config, VisualKind visualKind)
	: Entity(assets, world, texture, config.visualScale, config.collisionRadius)
	, speed(config.speed)
	, damage(config.damage)
	, knockback(config.knockback)
	, visualKind(visualKind)
{
}

void Shot::Update(float deltaTime)
{
	Move(deltaTime);

	const sf::Vector2f position{ GetPosition() };
	const sf::Vector2f velocity{ GetVelocity() };
	const float velocityLength{ std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y) };
	const sf::Vector2f direction{ velocityLength > 0.0001f
		? velocity / velocityLength
		: sf::Vector2f{ 0.f, -1.f } };
	GetWorld().AddEffectEvent({
		visualKind == VisualKind::Player
			? World::EffectEventType::PlayerProjectileGlow
			: World::EffectEventType::EnemyProjectileGlow,
		position,
		direction });
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
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Player),
		VisualKind::Player)
{
	SetPosition(position);
	const float angle{ rotationDegrees * std::numbers::pi_v<float> / 180.f
		- std::numbers::pi_v<float> / 2.f };
	SetRotation(sf::degrees(rotationDegrees));
	const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
	SetDirection(direction);
	GetWorld().AddEffectEvent({ World::EffectEventType::PlayerMuzzleFlash,
		position, direction });
	GetWorld().AddSound(Config::Sound::PlayerShot);
}

Entity::Type PlayerShot::GetType() const noexcept { return Type::Projectile_Player; }

bool PlayerShot::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Enemy || other.GetType() == Type::Asteroid)
		&& CheckCollision(other);
}

SaucerShot::SaucerShot(AssetStore& assets, World& world,
	const sf::Vector2f& position, const sf::Vector2f& targetPosition)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::EnemySaucerShot),
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Enemy),
		VisualKind::Enemy)
{
	SetPosition(position);
	const sf::Vector2f toTarget{ targetPosition - position };
	const float finalAngle{ std::atan2(toTarget.y, toTarget.x) };
	const sf::Vector2f direction{ std::cos(finalAngle), std::sin(finalAngle) };
	SetDirection(direction);
	GetWorld().AddEffectEvent({ World::EffectEventType::EnemyMuzzleFlash,
		position, direction });
	SetRotation(sf::degrees(finalAngle * 180.f / std::numbers::pi_v<float> + 90.f));
	GetWorld().AddSound(Config::Sound::EnemyShot);
}

Entity::Type SaucerShot::GetType() const noexcept { return Type::Projectile_Enemy; }

bool SaucerShot::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Player || other.GetType() == Type::Asteroid)
		&& CheckCollision(other);
}
