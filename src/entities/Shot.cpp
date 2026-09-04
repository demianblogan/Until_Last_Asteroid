#include "Shot.h"

#include <cmath>
#include <utility>
#include "assets/Assets.h"
#include "utils/ConfigEnums.h"
#include "utils/VectorMath.h"
#include "core/world/World.h"
#include "gameplay/GameplaySession.h"
#include "entities/enemies/Enemy.h"

Shot::Shot(Assets& assets, World& world, sf::Texture& texture, const GameplayData::ProjectileConfig& config,
	VisualKind visualKind, std::uint64_t attackID)
	: Entity(assets, world, texture, config.visualScale, config.collisionRadius)
	, speed(config.speed)
	, damage(config.damage)
	, knockback(config.knockback)
	, visualKind(visualKind)
	, playerAttackID(attackID)
{}

namespace
{
	// Which glow effect a shot's trail spawns, per VisualKind. Player and
	// Helper shots share the same plain glow; homing and triple-shot bullets
	// get their own distinct look; everything else (Enemy) gets the enemy glow.
	[[nodiscard]] Rendering::EffectEventType GetTrailGlowType(Shot::VisualKind visualKind) noexcept
	{
		switch (visualKind)
		{
		case Shot::VisualKind::Player:
		case Shot::VisualKind::Helper:
			return Rendering::EffectEventType::PlayerProjectileGlow;
		case Shot::VisualKind::PlayerHoming:
			return Rendering::EffectEventType::PlayerHomingProjectileGlow;
		case Shot::VisualKind::PlayerTriple:
			return Rendering::EffectEventType::PlayerTripleProjectileGlow;
		case Shot::VisualKind::Enemy:
			return Rendering::EffectEventType::EnemyProjectileGlow;
		default:
			std::unreachable();
		}
	}
}

void Shot::Update(float deltaTime)
{
	Move(deltaTime);

	const sf::Vector2f position{ GetPosition() };

	// SetHeading keeps the sprite's facing in lock-step with the velocity, so
	// the shot's forward direction already is its direction of travel -- no
	// need to re-derive it from the velocity vector.
	GetWorld().Effects().Add({ GetTrailGlowType(visualKind), position, GetForwardDirection() });

	const World& currentWorld = GetWorld();

	if (position.x < 0.f || position.x > currentWorld.GetWidth() ||
		position.y < 0.f || position.y > currentWorld.GetHeight())
	{
		Destroy();
	}
}

int Shot::GetDamage() const noexcept
{
	return damage;
}

float Shot::GetKnockback() const noexcept
{
	return knockback;
}

std::uint64_t Shot::GetPlayerAttackID() const noexcept
{
	return playerAttackID;
}

void Shot::ReflectToward(const sf::Vector2f& targetPosition)
{
	SetHeading(targetPosition - GetPosition());
	SetPresentation(1.f, 1.f, sf::Color(255, 70, 90));

	visualKind = VisualKind::Enemy;
	isReflected = true;
}

bool Shot::IsReflected() const noexcept
{
	return isReflected;
}

void Shot::SetHeading(const sf::Vector2f& direction) noexcept
{
	const sf::Vector2f unitDirection{ VectorMath::Normalize(direction, { 0.f, -1.f }) };
	SetVelocity(unitDirection * speed);
	// Sprite art points up at rotation 0, so facing = heading angle + 90.
	SetRotation(unitDirection.angle() + sf::degrees(90.f));
}

void Shot::SteerToward(const sf::Vector2f& targetPosition, float turnRateDegrees, float deltaTime) noexcept
{
	SetHeading(VectorMath::RotateToward(
		GetVelocity(),
		targetPosition - GetPosition(),
		sf::degrees(turnRateDegrees * deltaTime),
		GetForwardDirection()));
}

PlayerShot::PlayerShot(Assets& assets, World& world, const sf::Vector2f& position, const sf::Vector2f& aimDirection,
	std::uint64_t attackID, bool needToPlaySound, bool isTripleShotVisual)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::PlayerShot),
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Player),
		isTripleShotVisual ? VisualKind::PlayerTriple : world.GetSession().IsHomingBulletsActive()
		? VisualKind::PlayerHoming : VisualKind::Player,
		attackID)
	, isHomingEnabled(world.GetSession().IsHomingBulletsActive())
{
	SetPosition(position);
	SetHeading(aimDirection);

	const sf::Vector2f direction{ VectorMath::Normalize(aimDirection, { 0.f, -1.f }) };

	if (isHomingEnabled)
	{
		SetPresentation(1.f, 1.f, sf::Color(255, 205, 85));
		AcquireHomingTarget();
	}

	if (isTripleShotVisual)
		SetPresentation(1.f, 1.f, sf::Color(65, 255, 115));

	GetWorld().Effects().Add({ Rendering::EffectEventType::PlayerMuzzleFlash, position, direction });

	if (needToPlaySound)
	{
		const float shotPitch = isTripleShotVisual ? 0.82f : isHomingEnabled ? 1.18f : 1.f;
		GetWorld().Sound().AddSound(Config::Sound::PlayerShot, shotPitch);
	}
}

void PlayerShot::Update(float deltaTime)
{
	if (isHomingEnabled && !IsReflected())
		UpdateHoming(deltaTime);

	Shot::Update(deltaTime);
}

void PlayerShot::AcquireHomingTarget()
{
	const auto& config = GetAssets().GetGameplayData().GetPickups();
	// FindHomingTarget wants the cosine of the half-cone angle as its "how far
	// off my heading may a target sit" threshold (a dot-product test on the
	// far end).
	const float coneCosine = std::cos(sf::degrees(config.homingConeDegrees * 0.5f).asRadians());

	homingTarget = GetWorld().FindHomingTarget(GetPosition(), GetVelocity(), coneCosine);
	bossHomingTarget =
		GetWorld().BossHomingTargets().FindClosest(GetPosition(), GetVelocity(), coneCosine);

	if (homingTarget != nullptr && bossHomingTarget)
	{
		const auto bossPosition = GetWorld().BossHomingTargets().Get(*bossHomingTarget);

		if (!bossPosition)
		{
			bossHomingTarget.reset();
		}
		else
		{
			// Keep whichever candidate is closer; drop the other.
			const float toEntitySquared = (homingTarget->GetPosition() - GetPosition()).lengthSquared();
			const float toBossSquared = (*bossPosition - GetPosition()).lengthSquared();

			if (toBossSquared < toEntitySquared)
				homingTarget = nullptr;
			else
				bossHomingTarget.reset();
		}
	}
}

void PlayerShot::UpdateHoming(float deltaTime)
{
	// The boss-part target is an index whose world position can vanish
	// between frames (part destroyed), so it's always looked up fresh.
	const auto bossTargetPosition = [this]() -> std::optional<sf::Vector2f>
	{
		return bossHomingTarget
			? GetWorld().BossHomingTargets().Get(*bossHomingTarget)
			: std::nullopt;
	};

	if (!GetWorld().IsEntityActive(homingTarget) && !bossTargetPosition())
	{
		homingTarget = nullptr;
		bossHomingTarget.reset();
		AcquireHomingTarget();
	}

	const std::optional<sf::Vector2f> bossPosition{ bossTargetPosition() };
	if (homingTarget == nullptr && !bossPosition)
		return;

	const sf::Vector2f targetPosition{ bossPosition ? *bossPosition : homingTarget->GetPosition() };
	SteerToward(targetPosition, GetAssets().GetGameplayData().GetPickups().homingTurnSpeedDegrees, deltaTime);
}

Entity::Type PlayerShot::GetType() const noexcept
{
	return IsReflected() ? Type::Projectile_Enemy : Type::Projectile_Player;
}

bool PlayerShot::IsCollidingWith(const Entity& other) const
{
	if (IsReflected())
		return (other.GetType() == Type::Player || other.GetType() == Type::Asteroid) && CheckCollision(other);
	else if (other.GetType() == Type::Enemy || other.GetType() == Type::Asteroid)
		return static_cast<const Enemy&>(other).CollidesWithPlayerProjectile(*this);
	else
		return other.GetType() == Type::EnemyMissile && CheckCollision(other);
}

SaucerShot::SaucerShot(Assets& assets, World& world, const sf::Vector2f& position,
	const sf::Vector2f& targetPosition, GameplayData::ProjectileKind projectileKind, bool needToPlaySound)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::EnemySaucerShot),
		assets.GetGameplayData().GetProjectile(projectileKind), VisualKind::Enemy)
{
	SetPosition(position);
	SetHeading(targetPosition - position);

	GetWorld().Effects().Add({
		Rendering::EffectEventType::EnemyMuzzleFlash, position, GetForwardDirection() });

	if (needToPlaySound)
		GetWorld().Sound().AddSound(Config::Sound::EnemyShot);
}

Entity::Type SaucerShot::GetType() const noexcept
{
	return Type::Projectile_Enemy;
}

bool SaucerShot::IsCollidingWith(const Entity& other) const
{
	return (other.GetType() == Type::Player || other.GetType() == Type::Asteroid) && CheckCollision(other);
}

HelperShot::HelperShot(Assets& assets, World& world, const sf::Vector2f& position, const Entity* target)
	: Shot(assets, world, assets.Textures().Get(Config::Texture::PlayerShot),
		assets.GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Helper), VisualKind::Helper)
	, homingTarget(target)
{
	SetPosition(position);
	SetPresentation(0.92f, 1.f, sf::Color(75, 245, 255));

	SetHeading(target != nullptr ? target->GetPosition() - position : sf::Vector2f{ 1.f, 0.f });

	GetWorld().Effects().Add({
		Rendering::EffectEventType::PlayerMuzzleFlash, position, GetForwardDirection() });

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

bool HelperShot::IsCollidingWith(const Entity& other) const
{
	if (IsReflected())
		return (other.GetType() == Type::Player || other.GetType() == Type::Asteroid) && CheckCollision(other);
	else if (other.GetType() == Type::Enemy || other.GetType() == Type::Asteroid)
		return static_cast<const Enemy&>(other).CollidesWithPlayerProjectile(*this);
	else
		return other.GetType() == Type::EnemyMissile && CheckCollision(other);
}

void HelperShot::AcquireTarget()
{
	homingTarget = GetWorld().FindHomingTarget(GetPosition(), GetVelocity(), -1.f);
}

void HelperShot::UpdateHoming(float deltaTime)
{
	if (!GetWorld().IsEntityActive(homingTarget))
		AcquireTarget();

	if (homingTarget == nullptr)
		return;

	SteerToward(homingTarget->GetPosition(),
		GetAssets().GetGameplayData().GetPickups().helperBotTurnSpeedDegrees, deltaTime);
}
