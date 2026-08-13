#include "Shot.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"
#include "core/World.h"
#include "game/GameplaySession.h"

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
			: visualKind == VisualKind::PlayerHoming
				? World::EffectEventType::PlayerHomingProjectileGlow
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
		world.GetSession().IsHomingBulletsActive()
			? VisualKind::PlayerHoming
			: VisualKind::Player)
	, homingEnabled(world.GetSession().IsHomingBulletsActive())
{
	SetPosition(position);
	const float angle{ rotationDegrees * std::numbers::pi_v<float> / 180.f
		- std::numbers::pi_v<float> / 2.f };
	SetRotation(sf::degrees(rotationDegrees));
	const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
	SetDirection(direction);
	if (homingEnabled)
	{
		SetPresentation(1.f, 1.f, sf::Color(255, 205, 85));
		AcquireHomingTarget();
	}
	GetWorld().AddEffectEvent({ World::EffectEventType::PlayerMuzzleFlash,
		position, direction });
	GetWorld().AddSound(Config::Sound::PlayerShot);
}

void PlayerShot::Update(float deltaTime)
{
	if (homingEnabled)
		UpdateHoming(deltaTime);
	Shot::Update(deltaTime);
}

void PlayerShot::AcquireHomingTarget()
{
	const auto& config{ GetAssets().GetGameplayData().GetPickups() };
	const float halfConeRadians{
		config.homingConeDegrees * 0.5f * std::numbers::pi_v<float> / 180.f };
	homingTarget = GetWorld().FindHomingTarget(
		GetPosition(), GetVelocity(), std::cos(halfConeRadians));
}

void PlayerShot::UpdateHoming(float deltaTime)
{
	if (!GetWorld().IsEntityActive(homingTarget))
		AcquireHomingTarget();
	if (homingTarget == nullptr)
		return;

	const sf::Vector2f velocity{ GetVelocity() };
	const sf::Vector2f toTarget{ homingTarget->GetPosition() - GetPosition() };
	if ((velocity.x * velocity.x + velocity.y * velocity.y) <= 0.0001f ||
		(toTarget.x * toTarget.x + toTarget.y * toTarget.y) <= 0.0001f)
	{
		return;
	}

	const float currentAngle{ std::atan2(velocity.y, velocity.x) };
	const float targetAngle{ std::atan2(toTarget.y, toTarget.x) };
	const float angleDifference{ std::atan2(
		std::sin(targetAngle - currentAngle),
		std::cos(targetAngle - currentAngle)) };
	const float turnSpeedRadians{
		GetAssets().GetGameplayData().GetPickups().homingTurnSpeedDegrees *
		std::numbers::pi_v<float> / 180.f };
	const float maximumTurn{ turnSpeedRadians * deltaTime };
	const float finalAngle{ currentAngle + std::clamp(
		angleDifference, -maximumTurn, maximumTurn) };
	SetDirection({ std::cos(finalAngle), std::sin(finalAngle) });
	SetRotation(sf::radians(finalAngle + std::numbers::pi_v<float> * 0.5f));
}

Entity::Type PlayerShot::GetType() const noexcept { return Type::Projectile_Player; }

bool PlayerShot::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Enemy ||
		other.GetType() == Type::Asteroid ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

SaucerShot::SaucerShot(AssetStore& assets, World& world,
	const sf::Vector2f& position, const sf::Vector2f& targetPosition,
	GameplayData::ProjectileKind projectileKind, bool playSound)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::EnemySaucerShot),
		assets.GetGameplayData().GetProjectile(projectileKind),
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
	if (playSound)
		GetWorld().AddSound(Config::Sound::EnemyShot);
}

Entity::Type SaucerShot::GetType() const noexcept { return Type::Projectile_Enemy; }

bool SaucerShot::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Player || other.GetType() == Type::Asteroid)
		&& CheckCollision(other);
}
