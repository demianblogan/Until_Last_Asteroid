#include "ReflectorGunship.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "core/Collision.h"
#include "utils/ConfigEnums.h"
#include "utils/Pulse.h"
#include "utils/VectorMath.h"

ReflectorGunship::ReflectorGunship(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::ReflectorGunship),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ReflectorGunship))
{
	const auto& config = assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ReflectorGunship);

	shootInterval = std::max(0.05f, config.actionInterval);
	shieldDuration = std::max(0.1f, config.shieldDuration);
	figureEightAmplitude = std::max(1.f, config.sineAmplitude);
	figureEightFrequency = std::max(0.01f, config.sineFrequency);
}

void ReflectorGunship::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	Enemy::ConfigureApproachTarget(target);
	figureEightCenter = target;
}

bool ReflectorGunship::IsShieldActive() const noexcept
{
	return isShieldActive;
}

float ReflectorGunship::GetShieldPulse() const noexcept
{
	// A brightness multiplier for the shield's visual glow, oscillating over
	// time (see Pulse::Value) -- purely cosmetic, doesn't affect whether the
	// shield is actually up (see IsShieldActive/isShieldActive). 7
	// radians/second, kept from ever dimming below 76% brightness.
	return Pulse::Value(shieldPhaseElapsed, 7.f, 0.76f, 0.24f);
}

float ReflectorGunship::GetShieldRadius() const noexcept
{
	return GetCollisionRadius() * 1.42f;
}

bool ReflectorGunship::IsProjectileReflectionActive() const noexcept
{
	return IsShieldActive();
}

bool ReflectorGunship::BlocksPlayerLaser() const noexcept
{
	return IsShieldActive();
}

bool ReflectorGunship::AcceptsKnockback() const noexcept
{
	return false;
}

bool ReflectorGunship::CollidesWithPlayerProjectile(const Entity& projectile) const
{
	if (!IsShieldActive())
		return CheckCollision(projectile);

	return Collision::Circle(
		GetPosition(),
		GetShieldRadius(),
		projectile.GetPosition(), projectile.GetCollisionRadius());
}

sf::Vector2f ReflectorGunship::GetPlayerProjectileImpactPosition(const Entity& projectile) const noexcept
{
	if (!IsShieldActive())
		return projectile.GetPosition();

	const sf::Vector2f offset{ projectile.GetPosition() - GetPosition() };
	return GetPosition() + VectorMath::Normalize(offset, { 0.f, -1.f }) * GetShieldRadius();
}

Entity::Type ReflectorGunship::GetType() const noexcept
{
	return Type::Enemy;
}

bool ReflectorGunship::IsCollidingWith(const Entity& other) const
{
	if (other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::Projectile_Ally)
	{
		return CollidesWithPlayerProjectile(other);
	}

	return (other.GetType() == Type::Player || other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

void ReflectorGunship::Update(float deltaTime)
{
	const sf::Vector2f playerPosition{ GetWorld().GetPlayerPosition() };
	TurnTowards(playerPosition, GetRotationSpeed(), deltaTime);

	if (isApproachingCenter)
	{
		// The return value (whether it just arrived) isn't needed -- the
		// else branch below naturally takes over the very next frame once
		// isApproachingCenter flips false, with nothing extra to do here.
		static_cast<void>(UpdateApproach(deltaTime));
	}
	else
	{
		UpdateFigureEight(deltaTime);
	}

	shieldPhaseElapsed += deltaTime;

	while (shieldPhaseElapsed >= shieldDuration)
	{
		shieldPhaseElapsed -= shieldDuration;
		isShieldActive = !isShieldActive;
	}

	shootTimer += deltaTime;

	while (shootTimer >= shootInterval)
	{
		shootTimer -= shootInterval;
		ShootDoubleVolley();
	}
}

void ReflectorGunship::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion, GetSoundPitchMultiplier());
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion,GetPosition(), GetVelocity(), 1.45f });
}

void ReflectorGunship::UpdateFigureEight(float deltaTime)
{
	movementPhase = std::fmod(movementPhase + figureEightFrequency * deltaTime, 2.f * std::numbers::pi_v<float>);
	const float verticalAmplitude = figureEightAmplitude * 0.52f;
	const sf::Vector2f previousPosition{ GetPosition() };
	const sf::Vector2f nextPosition
	{
		figureEightCenter.x + figureEightAmplitude * std::sin(movementPhase),
		figureEightCenter.y + verticalAmplitude * std::sin(2.f * movementPhase)
	};

	SetPosition(nextPosition);

	if (deltaTime > 0.f)
		SetVelocity((nextPosition - previousPosition) / deltaTime);
}

void ReflectorGunship::ShootDoubleVolley()
{
	const auto& emitters = GetWeaponEmitters();
	for (std::size_t index = 0; index < emitters.size(); index++)
	{
		const sf::Vector2f muzzle{ GetWeaponEmitterPosition(index) };
		GetWorld().SpawnSaucerShot(
			muzzle, muzzle + GetForwardDirection() * 1000.f,
			GameplayData::ProjectileKind::Enemy, index == 0u);
	}
}
