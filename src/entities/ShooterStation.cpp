#include "ShooterStation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "core/Collision.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"

ShooterStation::ShooterStation(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::ShooterStation),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ShooterStation))
{
	const auto& config{
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::ShooterStation) };
	spawnAnimationDuration = config.spawnAnimationDuration;
	shieldDuration = spawnAnimationDuration;
	creationRemaining = shieldDuration;
	spawnElapsed = 0.f;
}

void ShooterStation::ConfigurePath(sf::Vector2f first, sf::Vector2f second)
{
	pathStart = first;
	pathEnd = second;
	targetPoint = pathStart;
	const sf::Vector2f route{ pathEnd - pathStart };
	const float routeLength{ std::sqrt(route.x * route.x + route.y * route.y) };
	const sf::Vector2f routeDirection{ routeLength > 0.001f
		? route / routeLength
		: sf::Vector2f{ 1.f, 0.f } };
	SetPosition(pathStart - routeDirection * (GetCollisionRadius() * 2.5f));
	const sf::Vector2f delta{ targetPoint - GetPosition() };
	const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
	SetVelocity(length > 0.001f ? delta / length * GetMovementSpeed() : sf::Vector2f{});
	pathConfigured = true;
	arriving = true;
}

void ShooterStation::ConfigureStationaryArrival(
	sf::Vector2f start,
	sf::Vector2f destination)
{
	pathStart = destination;
	pathEnd = destination;
	targetPoint = destination;
	SetPosition(start);
	const sf::Vector2f delta{ destination - start };
	const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
	SetVelocity(length > 0.001f
		? delta / length * GetMovementSpeed()
		: sf::Vector2f{});
	pathConfigured = true;
	arriving = true;
}

bool ShooterStation::IsShieldActive() const noexcept
{
	return !destructionActive && creationRemaining > 0.f;
}

float ShooterStation::GetShieldPulse() const noexcept
{
	return 0.72f + 0.28f * std::abs(std::sin(creationRemaining * 7.f));
}

float ShooterStation::GetSpawnChargeRatio() const noexcept
{
	return creationRemaining > 0.f
		? 1.f - creationRemaining / spawnAnimationDuration
		: 0.f;
}

bool ShooterStation::IsArriving() const noexcept { return arriving; }

bool ShooterStation::TakeDamage(int damage)
{
	if (destructionActive || IsShieldActive())
		return false;
	const bool destructionStarted{ Enemy::TakeDamage(damage) };
	// Scoring and drops are deferred until the visible destruction sequence
	// reaches its final explosion.
	return destructionStarted && !destructionActive;
}

Entity::Type ShooterStation::GetType() const noexcept { return Type::Enemy; }
bool ShooterStation::AcceptsKnockback() const noexcept { return false; }

bool ShooterStation::CollidesWithPlayerProjectile(
	const Entity& projectile) const
{
	if (!IsShieldActive())
		return CheckCollision(projectile);
	return Collision::Circle(
		GetPosition(), GetShieldRadius(),
		projectile.GetPosition(), projectile.GetCollisionRadius());
}

sf::Vector2f ShooterStation::GetPlayerProjectileImpactPosition(
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

float ShooterStation::GetShieldRadius() const noexcept
{
	return GetCollisionRadius() * 1.48f;
}

bool ShooterStation::IsCollideWith(const Entity& other) const
{
	if (destructionActive)
		return false;
	if (other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::Projectile_Ally)
		return CollidesWithPlayerProjectile(other);
	return (other.GetType() == Type::Player ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

void ShooterStation::Update(float deltaTime)
{
	if (destructionActive)
	{
		destructionElapsed += deltaTime;
		destructionRemaining = std::max(0.f, destructionRemaining - deltaTime);
		const float intensity{ 1.f + (1.f - destructionRemaining) * 1.8f };
		SetPosition(destructionOrigin + sf::Vector2f{
			std::sin(destructionElapsed * 68.f) * 4.f * intensity,
			std::cos(destructionElapsed * 83.f) * 3.2f * intensity });
		destructionExplosionAccumulator += deltaTime;
		constexpr float ExplosionInterval{ 0.14f };
		while (destructionExplosionAccumulator >= ExplosionInterval)
		{
			destructionExplosionAccumulator -= ExplosionInterval;
			const sf::Vector2f offset{
				Random::Float(-0.62f, 0.62f) * GetCollisionRadius(),
				Random::Float(-0.62f, 0.62f) * GetCollisionRadius() };
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
	if (pathConfigured && ReachedTarget())
	{
		SetPosition(targetPoint);
		if (arriving)
		{
			arriving = false;
			targetPoint = pathEnd;
			creationRemaining = shieldDuration;
			weldingAccumulator = 0.f;
			workingSoundHandle = GetWorld().Sound().AddSound(
				Config::Sound::EnemyStationWorking);
			GetWorld().SpawnStationShooter(
				GetPosition(), spawnAnimationDuration, this);
		}
		else
		{
			targetPoint = targetPoint == pathEnd ? pathStart : pathEnd;
		}
		const sf::Vector2f delta{ targetPoint - GetPosition() };
		const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
		SetVelocity(length > 0.001f
			? delta / length * GetMovementSpeed()
			: sf::Vector2f{});
	}
	SetRotation(GetRotation() + sf::degrees(5.f * deltaTime));
	if (arriving)
		return;

	if (creationRemaining > 0.f)
	{
		creationRemaining = std::max(0.f, creationRemaining - deltaTime);
		weldingAccumulator += deltaTime;
		constexpr float WeldingInterval{ 0.075f };
		while (weldingAccumulator >= WeldingInterval)
		{
			weldingAccumulator -= WeldingInterval;
			GetWorld().Effects().Add({
				Rendering::EffectEventType::StationWelding,
				GetPosition(), {}, 1.f });
		}
		if (creationRemaining <= 0.f)
		{
			GetWorld().Sound().StopSound(workingSoundHandle);
			workingSoundHandle = 0u;
		}
	}

	spawnElapsed += deltaTime;
	if (creationRemaining <= 0.f && spawnElapsed >= GetActionInterval())
	{
		spawnElapsed -= GetActionInterval();
		creationRemaining = shieldDuration;
		weldingAccumulator = 0.f;
		workingSoundHandle = GetWorld().Sound().AddSound(Config::Sound::EnemyStationWorking);
		GetWorld().SpawnStationShooter(
			GetPosition(), spawnAnimationDuration, this);
	}
}

void ShooterStation::BeginDestruction()
{
	if (destructionActive)
		return;
	destructionActive = true;
	destructionOrigin = GetPosition();
	destructionRemaining = 1.f;
	destructionExplosionAccumulator = 0.f;
	destructionElapsed = 0.f;
	SetVelocity({});
	GetWorld().Sound().StopSound(workingSoundHandle);
	workingSoundHandle = 0u;
}

bool ShooterStation::ReachedTarget() const noexcept
{
	const sf::Vector2f remaining{ targetPoint - GetPosition() };
	const sf::Vector2f currentVelocity{ GetVelocity() };
	return remaining.x * currentVelocity.x + remaining.y * currentVelocity.y <= 0.f;
}

void ShooterStation::OnDestroy()
{
	GetWorld().Sound().StopSound(workingSoundHandle);
	workingSoundHandle = 0u;
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion, 0.58f);
	GetWorld().Effects().Add({ Rendering::EffectEventType::StationExplosion,
		GetPosition(), {}, 2.f });
}
