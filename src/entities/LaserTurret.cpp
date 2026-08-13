#include "LaserTurret.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <SFML/Audio/SoundBuffer.hpp>

#include "assets/AssetStore.h"
#include "core/World.h"
#include "utils/ConfigEnums.h"

namespace
{
	sf::Vector2f Normalize(const sf::Vector2f& value)
	{
		const float length{ std::sqrt(value.x * value.x + value.y * value.y) };
		return length > 0.001f ? value / length : sf::Vector2f{ 1.f, 0.f };
	}
}

LaserTurret::LaserTurret(AssetStore& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::LaserTurret),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::LaserTurret))
{
	const auto& config{
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::LaserTurret) };
	beamDamage = config.beamDamage;
	beamWidth = config.beamWidth;
	laserDuration = std::max(0.1f, assets.Sounds().Get(
		Config::Sound::EnemyLaserShot).getDuration().asSeconds());
}

void LaserTurret::ConfigurePath(
	sf::Vector2f first, sf::Vector2f second, sf::Vector2f beamDirection)
{
	pathStart = first;
	pathEnd = second;
	inward = Normalize(beamDirection);
	targetCorner = pathStart;
	const sf::Vector2f routeDirection{ Normalize(pathEnd - pathStart) };
	SetPosition(pathStart - routeDirection * (GetCollisionRadius() * 2.5f));
	SetVelocity(routeDirection * GetMovementSpeed());
	phase = Phase::Arriving;
	atFirstCorner = false;
	const float angle{ std::atan2(inward.y, inward.x) +
		std::numbers::pi_v<float> * 0.5f };
	SetRotation(sf::radians(angle));
}

sf::Vector2f LaserTurret::GetBeamStart() const noexcept
{
	return GetPosition() + inward * (GetCollisionRadius() * 0.6f);
}

sf::Vector2f LaserTurret::GetBeamEnd() const noexcept
{
	const sf::Vector2f start{ GetBeamStart() };
	const float distance{ inward.x != 0.f
		? (inward.x > 0.f ? GetWorld().GetWidth() - start.x : start.x)
		: (inward.y > 0.f ? GetWorld().GetHeight() - start.y : start.y) };
	return start + inward * distance;
}

float LaserTurret::GetBeamWidth() const noexcept { return beamWidth; }
float LaserTurret::GetBeamPulse() const noexcept
{
	return IsBeamActive()
		? 0.72f + 0.28f * std::abs(std::sin(beamTime * 8.f))
		: 0.f;
}
float LaserTurret::GetBeamAnimationTime() const noexcept { return beamTime; }
bool LaserTurret::IsBeamActive() const noexcept { return phase == Phase::Traversing; }
bool LaserTurret::AcceptsKnockback() const noexcept { return false; }
Entity::Type LaserTurret::GetType() const noexcept { return Type::Enemy; }

bool LaserTurret::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Player ||
		other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

void LaserTurret::Update(float deltaTime)
{
	if (phase == Phase::Waiting)
	{
		waitRemaining = std::max(0.f, waitRemaining - deltaTime);
		if (waitRemaining > 0.f)
			return;
		BeginTraversal();
	}

	if (phase == Phase::Arriving)
	{
		beamTime += deltaTime;
		Move(deltaTime);
		if (ReachedTarget(targetCorner))
		{
			SetPosition(targetCorner);
			SetVelocity({});
			atFirstCorner = true;
			phase = Phase::Waiting;
			waitRemaining = 0.5f;
		}
		return;
	}

	beamTime += deltaTime;
	traversalElapsed = std::min(laserDuration, traversalElapsed + deltaTime);
	const float progress{ traversalElapsed / laserDuration };
	SetPosition(traversalOrigin + (targetCorner - traversalOrigin) * progress);
	GetWorld().DamagePlayerWithBeam(
		GetBeamStart(), GetBeamEnd(), beamWidth, beamDamage);

	if (traversalElapsed >= laserDuration)
	{
		GetWorld().StopSound(laserSoundHandle);
		laserSoundHandle = 0u;
		SetPosition(targetCorner);
		SetVelocity({});
		atFirstCorner = !atFirstCorner;
		phase = Phase::Waiting;
		waitRemaining = 1.f;
	}
}

void LaserTurret::BeginTraversal()
{
	phase = Phase::Traversing;
	beamTime = 0.f;
	targetCorner = atFirstCorner ? pathEnd : pathStart;
	traversalOrigin = GetPosition();
	traversalElapsed = 0.f;
	const sf::Vector2f delta{ targetCorner - GetPosition() };
	SetVelocity(delta / laserDuration);
	laserSoundHandle = GetWorld().AddSound(Config::Sound::EnemyLaserShot);
}

bool LaserTurret::ReachedTarget(sf::Vector2f target) const noexcept
{
	const sf::Vector2f remaining{ target - GetPosition() };
	const sf::Vector2f velocity{ GetVelocity() };
	return remaining.x * velocity.x + remaining.y * velocity.y <= 0.f;
}

void LaserTurret::OnDestroy()
{
	GetWorld().StopSound(laserSoundHandle);
	laserSoundHandle = 0u;
	GetWorld().AddSound(Config::Sound::ShipExplosion, 0.82f);
	GetWorld().AddEffectEvent({ World::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), 1.3f });
}
