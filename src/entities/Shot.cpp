#include "Shot.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include "assets/Assets.h"
#include "utils/ConfigEnums.h"
#include "core/world/World.h"
#include "gameplay/GameplaySession.h"
#include "entities/Enemy.h"

Shot::Shot(Assets& assets, World& world, sf::Texture& texture,
	const GameplayData::ProjectileConfig& config, VisualKind visualKind,
	std::uint64_t attackID)
	: Entity(assets, world, texture, config.visualScale, config.collisionRadius)
	, speed(config.speed)
	, damage(config.damage)
	, knockback(config.knockback)
	, visualKind(visualKind)
	, playerAttackID(attackID)
{
}

void Shot::Update(float deltaTime)
{
	Move(deltaTime);

	const sf::Vector2f position{ GetPosition() };
	const sf::Vector2f currentVelocity{ GetVelocity() };
	const float velocityLength{ std::sqrt(
		currentVelocity.x * currentVelocity.x + currentVelocity.y * currentVelocity.y) };
	const sf::Vector2f direction{ velocityLength > 0.0001f
		? currentVelocity / velocityLength
		: sf::Vector2f{ 0.f, -1.f } };
	GetWorld().Effects().Add({
		visualKind == VisualKind::Player
			? EffectEventType::PlayerProjectileGlow
			: visualKind == VisualKind::Helper
				? EffectEventType::PlayerProjectileGlow
			: visualKind == VisualKind::PlayerHoming
				? EffectEventType::PlayerHomingProjectileGlow
				: visualKind == VisualKind::PlayerTriple
					? EffectEventType::PlayerTripleProjectileGlow
				: EffectEventType::EnemyProjectileGlow,
		position,
		direction });
	const World& currentWorld{ GetWorld() };
	if (position.x < 0.f || position.x > currentWorld.GetWidth() ||
		position.y < 0.f || position.y > currentWorld.GetHeight())
	{
		Destroy();
	}
}

int Shot::GetDamage() const noexcept { return damage; }
float Shot::GetKnockback() const noexcept { return knockback; }
std::uint64_t Shot::GetPlayerAttackID() const noexcept { return playerAttackID; }

void Shot::ReflectToward(const sf::Vector2f& targetPosition)
{
	const sf::Vector2f toTarget{ targetPosition - GetPosition() };
	const float angle{ std::atan2(toTarget.y, toTarget.x) };
	SetDirection({ std::cos(angle), std::sin(angle) });
	SetRotation(sf::radians(angle + std::numbers::pi_v<float> * 0.5f));
	SetPresentation(1.f, 1.f, sf::Color(255, 70, 90));
	visualKind = VisualKind::Enemy;
	reflected = true;
}

bool Shot::IsReflected() const noexcept { return reflected; }

void Shot::SetDirection(const sf::Vector2f& direction) noexcept
{
	SetVelocity(direction * speed);
}

PlayerShot::PlayerShot(Assets& assets, World& world,
	const sf::Vector2f& position, float rotationDegrees, std::uint64_t attackID,
	bool playSound, bool tripleShotVisual)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::PlayerShot),
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Player),
		tripleShotVisual
			? VisualKind::PlayerTriple
			: world.GetSession().IsHomingBulletsActive()
			? VisualKind::PlayerHoming
			: VisualKind::Player,
		attackID)
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
	if (tripleShotVisual)
		SetPresentation(1.f, 1.f, sf::Color(65, 255, 115));
	GetWorld().Effects().Add({ EffectEventType::PlayerMuzzleFlash,
		position, direction });
	if (playSound)
	{
		const float shotPitch{ tripleShotVisual
			? 0.82f
			: homingEnabled ? 1.18f : 1.f };
		GetWorld().Sound().AddSound(Config::Sound::PlayerShot, shotPitch);
	}
}

void PlayerShot::Update(float deltaTime)
{
	if (homingEnabled && !IsReflected())
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
	bossHomingTarget = GetWorld().BossHomingTargets().FindClosest(
		GetPosition(), GetVelocity(), std::cos(halfConeRadians));
	if (homingTarget != nullptr && bossHomingTarget)
	{
		const auto bossPosition{
			GetWorld().BossHomingTargets().Get(*bossHomingTarget) };
		if (!bossPosition)
			bossHomingTarget.reset();
		else
		{
			const sf::Vector2f entityOffset{
				homingTarget->GetPosition() - GetPosition() };
			const sf::Vector2f bossOffset{ *bossPosition - GetPosition() };
			const float entityDistanceSquared{
				entityOffset.x * entityOffset.x + entityOffset.y * entityOffset.y };
			const float bossDistanceSquared{
				bossOffset.x * bossOffset.x + bossOffset.y * bossOffset.y };
			if (bossDistanceSquared < entityDistanceSquared)
				homingTarget = nullptr;
			else
				bossHomingTarget.reset();
		}
	}
}

void PlayerShot::UpdateHoming(float deltaTime)
{
	if (!GetWorld().IsEntityActive(homingTarget) &&
		(!bossHomingTarget ||
			!GetWorld().BossHomingTargets().Get(*bossHomingTarget)))
	{
		homingTarget = nullptr;
		bossHomingTarget.reset();
		AcquireHomingTarget();
	}
	const std::optional<sf::Vector2f> bossPosition{ bossHomingTarget
		? GetWorld().BossHomingTargets().Get(*bossHomingTarget)
		: std::nullopt };
	if (homingTarget == nullptr && !bossPosition)
		return;

	const sf::Vector2f currentVelocity{ GetVelocity() };
	const sf::Vector2f targetPosition{ bossPosition
		? *bossPosition
		: homingTarget->GetPosition() };
	const sf::Vector2f toTarget{ targetPosition - GetPosition() };
	if ((currentVelocity.x * currentVelocity.x + currentVelocity.y * currentVelocity.y) <= 0.0001f ||
		(toTarget.x * toTarget.x + toTarget.y * toTarget.y) <= 0.0001f)
	{
		return;
	}

	const float currentAngle{ std::atan2(currentVelocity.y, currentVelocity.x) };
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

Entity::Type PlayerShot::GetType() const noexcept
{
	return IsReflected() ? Type::Projectile_Enemy : Type::Projectile_Player;
}

bool PlayerShot::IsCollideWith(const Entity& other) const
{
	if (IsReflected())
		return (other.GetType() == Type::Player ||
			other.GetType() == Type::Asteroid) && CheckCollision(other);
	if (other.GetType() == Type::Enemy || other.GetType() == Type::Asteroid)
		return static_cast<const Enemy&>(other).CollidesWithPlayerProjectile(*this);
	return other.GetType() == Type::EnemyMissile && CheckCollision(other);
}

SaucerShot::SaucerShot(Assets& assets, World& world,
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
	GetWorld().Effects().Add({ EffectEventType::EnemyMuzzleFlash,
		position, direction });
	SetRotation(sf::degrees(finalAngle * 180.f / std::numbers::pi_v<float> + 90.f));
	if (playSound)
		GetWorld().Sound().AddSound(Config::Sound::EnemyShot);
}

Entity::Type SaucerShot::GetType() const noexcept { return Type::Projectile_Enemy; }

bool SaucerShot::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Player || other.GetType() == Type::Asteroid)
		&& CheckCollision(other);
}

HelperShot::HelperShot(Assets& assets, World& world,
	const sf::Vector2f& position, const Entity* target)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::PlayerShot),
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Helper),
		VisualKind::Helper)
	, homingTarget(target)
{
	SetPosition(position);
	SetPresentation(0.92f, 1.f, sf::Color(75, 245, 255));
	const sf::Vector2f toTarget{ target != nullptr
		? target->GetPosition() - position
		: sf::Vector2f{ 1.f, 0.f } };
	const float angle{ std::atan2(toTarget.y, toTarget.x) };
	SetDirection({ std::cos(angle), std::sin(angle) });
	SetRotation(sf::radians(angle + std::numbers::pi_v<float> * 0.5f));
	GetWorld().Effects().Add({
		EffectEventType::PlayerMuzzleFlash,
		position,
		{ std::cos(angle), std::sin(angle) } });
	GetWorld().Sound().AddSound(Config::Sound::PlayerShot, 1.35f);
}

void HelperShot::Update(float deltaTime)
{
	if (!IsReflected())
		UpdateHoming(deltaTime);
	Shot::Update(deltaTime);
}

Entity::Type HelperShot::GetType() const noexcept
{
	return IsReflected() ? Type::Projectile_Enemy : Type::Projectile_Ally;
}

bool HelperShot::IsCollideWith(const Entity& other) const
{
	if (IsReflected())
		return (other.GetType() == Type::Player ||
			other.GetType() == Type::Asteroid) && CheckCollision(other);
	if (other.GetType() == Type::Enemy || other.GetType() == Type::Asteroid)
		return static_cast<const Enemy&>(other).CollidesWithPlayerProjectile(*this);
	return other.GetType() == Type::EnemyMissile && CheckCollision(other);
}

void HelperShot::AcquireTarget()
{
	homingTarget = GetWorld().FindHomingTarget(
		GetPosition(), GetVelocity(), -1.f);
}

void HelperShot::UpdateHoming(float deltaTime)
{
	if (!GetWorld().IsEntityActive(homingTarget))
		AcquireTarget();
	if (homingTarget == nullptr)
		return;

	const sf::Vector2f currentVelocity{ GetVelocity() };
	const sf::Vector2f toTarget{ homingTarget->GetPosition() - GetPosition() };
	if ((currentVelocity.x * currentVelocity.x + currentVelocity.y * currentVelocity.y) <= 0.0001f ||
		(toTarget.x * toTarget.x + toTarget.y * toTarget.y) <= 0.0001f)
	{
		return;
	}

	const float currentAngle{ std::atan2(currentVelocity.y, currentVelocity.x) };
	const float targetAngle{ std::atan2(toTarget.y, toTarget.x) };
	const float angleDifference{ std::atan2(
		std::sin(targetAngle - currentAngle),
		std::cos(targetAngle - currentAngle)) };
	const float turnSpeedRadians{
		GetAssets().GetGameplayData().GetPickups().helperBotTurnSpeedDegrees *
		std::numbers::pi_v<float> / 180.f };
	const float maximumTurn{ turnSpeedRadians * deltaTime };
	const float finalAngle{ currentAngle + std::clamp(
		angleDifference, -maximumTurn, maximumTurn) };
	SetDirection({ std::cos(finalAngle), std::sin(finalAngle) });
	SetRotation(sf::radians(finalAngle + std::numbers::pi_v<float> * 0.5f));
}
