#include "ShooterStation.h"

#include <algorithm>
#include <cmath>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "core/Collision.h"
#include "utils/ConfigEnums.h"
#include "utils/Pulse.h"
#include "utils/Random.h"
#include "utils/VectorMath.h"

ShooterStation::ShooterStation(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::ShooterStation),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ShooterStation))
{
	const auto& config = assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ShooterStation);
	spawnAnimationDuration = config.spawnAnimationDuration;
	shieldDuration = spawnAnimationDuration;
	creationRemaining = shieldDuration;
	spawnElapsed = 0.f;
	spawnInterval = config.actionInterval;
}

void ShooterStation::ConfigurePath(sf::Vector2f first, sf::Vector2f second)
{
	pathStart = first;
	pathEnd = second;
	targetPoint = pathStart;

	const sf::Vector2f routeDirection{ VectorMath::Normalize(pathEnd - pathStart, { 1.f, 0.f }) };
	SetPosition(pathStart - routeDirection * (GetCollisionRadius() * 2.5f));

	SetVelocity(VectorMath::Normalize(targetPoint - GetPosition(), {}) * GetMovementSpeed());

	isPathConfigured = true;
	isArriving = true;
}

void ShooterStation::ConfigureStationaryArrival(sf::Vector2f start, sf::Vector2f destination)
{
	pathStart = destination;
	pathEnd = destination;
	targetPoint = destination;

	SetPosition(start);
	SetVelocity(VectorMath::Normalize(destination - start, {}) * GetMovementSpeed());

	isPathConfigured = true;
	isArriving = true;
}

bool ShooterStation::IsShieldActive() const noexcept
{
	return !isDestructionActive && creationRemaining > 0.f;
}

float ShooterStation::GetShieldPulse() const noexcept
{
	// Same oscillating-brightness trick as LaserTurret::GetBeamPulse and
	// ReflectorGunship::GetShieldPulse -- see Pulse::Value.
	return Pulse::Value(creationRemaining, 7.f, 0.72f, 0.28f);
}

float ShooterStation::GetSpawnChargeRatio() const noexcept
{
	return creationRemaining > 0.f ? 1.f - creationRemaining / spawnAnimationDuration : 0.f;
}

bool ShooterStation::IsArriving() const noexcept
{
	return isArriving;
}

bool ShooterStation::TakeDamage(int damage)
{
	if (isDestructionActive || IsShieldActive())
		return false;

	const bool isDestructionStarted = Enemy::TakeDamage(damage);

	// Scoring and drops are deferred until the visible destruction sequence
	// reaches its final explosion.
	return isDestructionStarted && !isDestructionActive;
}

bool ShooterStation::AcceptsKnockback() const noexcept
{
	return false;
}

bool ShooterStation::CollidesWithPlayerProjectile(const Entity& projectile) const
{
	if (!IsShieldActive())
		return CheckCollision(projectile);

	return Collision::Circle(
		GetPosition(),
		GetShieldRadius(),
		projectile.GetPosition(),
		projectile.GetCollisionRadius());
}

sf::Vector2f ShooterStation::GetPlayerProjectileImpactPosition(const Entity& projectile) const noexcept
{
	if (!IsShieldActive())
		return projectile.GetPosition();

	const sf::Vector2f offset{ projectile.GetPosition() - GetPosition() };
	return GetPosition() + VectorMath::Normalize(offset, { 0.f, -1.f }) * GetShieldRadius();
}

float ShooterStation::GetShieldRadius() const noexcept
{
	return GetCollisionRadius() * 1.48f;
}

bool ShooterStation::IsCollidingWith(const Entity& other) const
{
	if (isDestructionActive)
		return false;

	if (other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::Projectile_Ally)
		return CollidesWithPlayerProjectile(other);

	return (other.GetType() == Type::Player ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

void ShooterStation::Update(float deltaTime)
{
	if (isDestructionActive)
	{
		destructionElapsed += deltaTime;
		destructionRemaining = std::max(0.f, destructionRemaining - deltaTime);

		const float intensity = 1.f + (1.f - destructionRemaining) * 1.8f;

		SetPosition(destructionOrigin + sf::Vector2f{
			std::sin(destructionElapsed * 68.f) * 4.f * intensity,
			std::cos(destructionElapsed * 83.f) * 3.2f * intensity });

		destructionExplosionAccumulator += deltaTime;

		constexpr float ExplosionInterval = 0.14f;

		while (destructionExplosionAccumulator >= ExplosionInterval)
		{
			destructionExplosionAccumulator -= ExplosionInterval;

			const sf::Vector2f offset
			{
				Random::Float(-0.62f, 0.62f) * GetCollisionRadius(),
				Random::Float(-0.62f, 0.62f) * GetCollisionRadius()
			};

			GetWorld().Effects().Add({
				Rendering::EffectEventType::StationChainExplosion,
				GetPosition() + offset, {}, Random::Float(1.05f, 1.45f) });
		}

		if (destructionRemaining <= 0.f)
		{
			SetPosition(destructionOrigin);
			GetWorld().CompleteDelayedEnemyDestruction(*this);
			Destroy();
		}

		return;
	}

	Move(deltaTime);

	if (isPathConfigured && IsTargetReached())
	{
		SetPosition(targetPoint);

		if (isArriving)
		{
			isArriving = false;
			targetPoint = pathEnd;
			creationRemaining = shieldDuration;
			weldingAccumulator = 0.f;
			workingSoundHandle = GetWorld().Sound().AddSound(Config::Sound::EnemyStationWorking);

			GetWorld().SpawnStationShooter(GetPosition(), spawnAnimationDuration, this);
		}
		else
		{
			targetPoint = targetPoint == pathEnd ? pathStart : pathEnd;
		}

		SetVelocity(VectorMath::Normalize(targetPoint - GetPosition(), {}) * GetMovementSpeed());
	}

	SetRotation(GetRotation() + sf::degrees(5.f * deltaTime));

	if (isArriving)
		return;

	if (creationRemaining > 0.f)
	{
		creationRemaining = std::max(0.f, creationRemaining - deltaTime);
		weldingAccumulator += deltaTime;

		constexpr float WeldingInterval = 0.075f;

		while (weldingAccumulator >= WeldingInterval)
		{
			weldingAccumulator -= WeldingInterval;
			GetWorld().Effects().Add({ Rendering::EffectEventType::StationWelding,GetPosition(), {}, 1.f });
		}

		if (creationRemaining <= 0.f)
		{
			GetWorld().Sound().StopSound(workingSoundHandle);
			workingSoundHandle = 0u;
		}
	}

	spawnElapsed += deltaTime;

	if (creationRemaining <= 0.f && spawnElapsed >= spawnInterval)
	{
		spawnElapsed -= spawnInterval;
		creationRemaining = shieldDuration;
		weldingAccumulator = 0.f;
		workingSoundHandle = GetWorld().Sound().AddSound(Config::Sound::EnemyStationWorking);

		GetWorld().SpawnStationShooter(GetPosition(), spawnAnimationDuration, this);
	}
}

void ShooterStation::BeginDestruction()
{
	if (isDestructionActive)
		return;

	isDestructionActive = true;
	destructionOrigin = GetPosition();
	destructionRemaining = 1.f;
	destructionExplosionAccumulator = 0.f;
	destructionElapsed = 0.f;

	SetVelocity({});

	GetWorld().Sound().StopSound(workingSoundHandle);
	workingSoundHandle = 0u;
}

bool ShooterStation::IsTargetReached() const noexcept
{
	// Dot product <= 0: the target is no longer ahead along our heading.
	return (targetPoint - GetPosition()).dot(GetVelocity()) <= 0.f;
}

void ShooterStation::OnDestroy()
{
	GetWorld().Sound().StopSound(workingSoundHandle);
	workingSoundHandle = 0u;

	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion, 0.58f);
	GetWorld().Effects().Add({ Rendering::EffectEventType::StationExplosion, GetPosition(), {}, 2.f });
}