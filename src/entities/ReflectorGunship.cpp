#include "ReflectorGunship.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "core/Collision.h"
#include "utils/ConfigEnums.h"

ReflectorGunship::ReflectorGunship(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::ReflectorGunship),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ReflectorGunship))
{
	const auto& config{
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ReflectorGunship) };
	shootInterval = std::max(0.05f, config.actionInterval);
	shieldDuration = std::max(0.1f, config.shieldDuration);
	figureEightAmplitude = std::max(1.f, config.sineAmplitude);
	figureEightFrequency = std::max(0.01f, config.sineFrequency);
}

void ReflectorGunship::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	approachTarget = target;
	figureEightCenter = target;
	approachingCenter = true;
}

bool ReflectorGunship::IsShieldActive() const noexcept
{
	return shieldActive;
}

float ReflectorGunship::GetShieldPulse() const noexcept
{
	return 0.76f + 0.24f * std::abs(std::sin(shieldPhaseElapsed * 7.f));
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

bool ReflectorGunship::CollidesWithPlayerProjectile(
	const Entity& projectile) const
{
	if (!IsShieldActive())
		return CheckCollision(projectile);
	return Collision::Circle(
		GetPosition(), GetShieldRadius(),
		projectile.GetPosition(), projectile.GetCollisionRadius());
}

sf::Vector2f ReflectorGunship::GetPlayerProjectileImpactPosition(
	const Entity& projectile) const noexcept
{
	if (!IsShieldActive())
		return projectile.GetPosition();
	const sf::Vector2f offset{ projectile.GetPosition() - GetPosition() };
	const float length{ std::sqrt(offset.x * offset.x + offset.y * offset.y) };
	const sf::Vector2f direction{ length > 0.001f
		? offset / length
		: sf::Vector2f{ 0.f, -1.f } };
	return GetPosition() + direction * GetShieldRadius();
}

Entity::Type ReflectorGunship::GetType() const noexcept
{
	return Type::Enemy;
}

bool ReflectorGunship::IsCollideWith(const Entity& other) const
{
	if (other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::Projectile_Ally)
	{
		return CollidesWithPlayerProjectile(other);
	}
	return (other.GetType() == Type::Player ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

void ReflectorGunship::Update(float deltaTime)
{
	const sf::Vector2f playerPosition{ GetWorld().GetPlayerPosition() };
	TurnTowards(playerPosition, GetRotationSpeed(), deltaTime);

	if (approachingCenter)
	{
		if (MoveToApproachTarget(deltaTime))
			approachingCenter = false;
	}
	else
	{
		UpdateFigureEight(deltaTime);
	}

	shieldPhaseElapsed += deltaTime;
	while (shieldPhaseElapsed >= shieldDuration)
	{
		shieldPhaseElapsed -= shieldDuration;
		shieldActive = !shieldActive;
	}

	shootTimer += deltaTime;
	while (shootTimer >= shootInterval)
	{
		shootTimer -= shootInterval;
		ShootDoubleVolley(playerPosition);
	}
}

void ReflectorGunship::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion, GetSoundPitch());
	GetWorld().Effects().Add({
		Rendering::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), 1.45f });
}

bool ReflectorGunship::MoveToApproachTarget(float deltaTime)
{
	const sf::Vector2f delta{ approachTarget - GetPosition() };
	const float distance{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
	const float step{ GetMovementSpeed() * deltaTime };
	if (distance <= std::max(step, 0.001f))
	{
		SetPosition(approachTarget);
		SetVelocity({});
		return true;
	}
	SetVelocity(delta / distance * GetMovementSpeed());
	Move(deltaTime);
	return false;
}

void ReflectorGunship::UpdateFigureEight(float deltaTime)
{
	movementPhase = std::fmod(
		movementPhase + figureEightFrequency * deltaTime,
		2.f * std::numbers::pi_v<float>);
	const float verticalAmplitude{ figureEightAmplitude * 0.52f };
	const sf::Vector2f previousPosition{ GetPosition() };
	const sf::Vector2f nextPosition{
		figureEightCenter.x + figureEightAmplitude * std::sin(movementPhase),
		figureEightCenter.y + verticalAmplitude * std::sin(2.f * movementPhase) };
	SetPosition(nextPosition);
	if (deltaTime > 0.f)
		SetVelocity((nextPosition - previousPosition) / deltaTime);
}

void ReflectorGunship::ShootDoubleVolley(const sf::Vector2f& playerPosition)
{
	(void)playerPosition;
	const auto& emitters{ GetWeaponEmitters() };
	for (std::size_t index{ 0 }; index < emitters.size(); ++index)
	{
		const sf::Vector2f muzzle{ GetWeaponEmitterPosition(index) };
		GetWorld().SpawnSaucerShot(
			muzzle, muzzle + GetForwardDirection() * 1000.f,
			GameplayData::ProjectileKind::Enemy, index == 0u);
	}
}

sf::Vector2f ReflectorGunship::GetWeaponEmitterPosition(std::size_t index) const
{
	const sf::Sprite& entitySprite{ GetSprite() };
	const sf::IntRect textureRect{ entitySprite.getTextureRect() };
	const GameplayData::NormalizedPoint& emitter{ GetWeaponEmitters().at(index) };
	const sf::Vector2f localPosition{
		static_cast<float>(textureRect.position.x) +
			static_cast<float>(textureRect.size.x) * emitter.x,
		static_cast<float>(textureRect.position.y) +
			static_cast<float>(textureRect.size.y) * emitter.y };
	return entitySprite.getTransform().transformPoint(localPosition);
}
