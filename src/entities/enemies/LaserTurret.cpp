#include "LaserTurret.h"

#include <algorithm>
#include <SFML/Audio/SoundBuffer.hpp>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Pulse.h"
#include "utils/VectorMath.h"

LaserTurret::LaserTurret(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::LaserTurret),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::LaserTurret))
{
	const auto& config = assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::LaserTurret);

	beamDamage = config.beamDamage;
	beamWidth = config.beamWidth;
	laserDuration = std::max(0.1f, assets.Sounds().Get(Config::Sound::EnemyLaserShot).getDuration().asSeconds());
}

void LaserTurret::ConfigurePath(sf::Vector2f first, sf::Vector2f second, sf::Vector2f beamDirection)
{
	pathStart = first;
	pathEnd = second;
	inward = VectorMath::Normalize(beamDirection);
	targetCorner = pathStart;

	const sf::Vector2f routeDirection{ VectorMath::Normalize(pathEnd - pathStart) };

	SetPosition(pathStart - routeDirection * (GetCollisionRadius() * 2.5f));
	SetVelocity(routeDirection * GetMovementSpeed());

	phase = Phase::Arriving;
	isAtFirstCorner = false;

	// Face the beam direction (sprite art points up at rotation 0, hence +90).
	SetRotation(inward.angle() + sf::degrees(90.f));
}

void LaserTurret::ConfigureStationaryArrival(sf::Vector2f start, sf::Vector2f destination, sf::Vector2f beamDirection)
{
	pathStart = destination;
	pathEnd = destination;
	inward = VectorMath::Normalize(beamDirection);
	targetCorner = destination;

	SetPosition(start);

	const sf::Vector2f routeDirection{ VectorMath::Normalize(destination - start) };
	SetVelocity(routeDirection * GetMovementSpeed());

	phase = Phase::Arriving;
	isAtFirstCorner = false;

	// Face the beam direction (sprite art points up at rotation 0, hence +90).
	SetRotation(inward.angle() + sf::degrees(90.f));
}

sf::Vector2f LaserTurret::GetBeamStart() const noexcept
{
	return GetPosition() + inward * (GetCollisionRadius() * 0.6f);
}

sf::Vector2f LaserTurret::GetBeamEnd() const noexcept
{
	const sf::Vector2f start{ GetBeamStart() };
	const float distance =
		(inward.x != 0.f)
		? (inward.x > 0.f ? GetWorld().GetWidth() - start.x : start.x)
		: (inward.y > 0.f ? GetWorld().GetHeight() - start.y : start.y);

	return start + inward * distance;
}

float LaserTurret::GetBeamWidth() const noexcept
{
	return beamWidth;
}

float LaserTurret::GetBeamPulse() const noexcept
{
	// A brightness multiplier for the beam's visual glow, oscillating over
	// time (see Pulse::Value) rather than staying at a flat intensity --
	// purely cosmetic, has no effect on damage. beamTime is the "clock"
	// (just counts seconds, see Update()); 8 radians/second is a bit over
	// one pulse per second; 0.72..1.0 keeps the glow from ever dimming all
	// the way to black. 0 whenever the beam isn't actually firing.
	return IsBeamActive() ? Pulse::Value(beamTime, 8.f, 0.72f, 0.28f) : 0.f;
}

float LaserTurret::GetBeamAnimationTime() const noexcept
{
	return beamTime;
}

bool LaserTurret::IsBeamActive() const noexcept
{
	return phase == Phase::Traversing;
}

bool LaserTurret::IsArriving() const noexcept
{
	return phase == Phase::Arriving;
}

bool LaserTurret::AcceptsKnockback() const noexcept
{
	return false;
}

Entity::Type LaserTurret::GetType() const noexcept
{
	return Type::Enemy;
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

			isAtFirstCorner = true;
			phase = Phase::Waiting;
			waitRemaining = 0.5f;
		}

		return;
	}

	beamTime += deltaTime;
	traversalElapsed = std::min(laserDuration, traversalElapsed + deltaTime);

	const float progress = traversalElapsed / laserDuration;
	SetPosition(traversalOrigin + (targetCorner - traversalOrigin) * progress);
	GetWorld().DamagePlayerWithBeam(GetBeamStart(), GetBeamEnd(), beamWidth, beamDamage);

	if (traversalElapsed >= laserDuration)
	{
		GetWorld().Sound().StopSound(laserSoundHandle);
		laserSoundHandle = 0u;

		SetPosition(targetCorner);
		SetVelocity({});

		isAtFirstCorner = !isAtFirstCorner;
		phase = Phase::Waiting;
		waitRemaining = 1.f;
	}
}

void LaserTurret::BeginTraversal()
{
	phase = Phase::Traversing;
	beamTime = 0.f;
	targetCorner = isAtFirstCorner ? pathEnd : pathStart;
	traversalOrigin = GetPosition();
	traversalElapsed = 0.f;

	const sf::Vector2f delta{ targetCorner - GetPosition() };
	SetVelocity(delta / laserDuration);

	laserSoundHandle = GetWorld().Sound().AddSound(Config::Sound::EnemyLaserShot);
}

bool LaserTurret::ReachedTarget(sf::Vector2f target) const noexcept
{
	// Dot product <= 0 means the target is no longer ahead of us along our
	// current heading -- i.e. we've reached or passed it.
	return (target - GetPosition()).dot(GetVelocity()) <= 0.f;
}

void LaserTurret::OnDestroy()
{
	GetWorld().Sound().StopSound(laserSoundHandle);
	laserSoundHandle = 0u;

	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion, 0.82f);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPosition(), GetVelocity(), 1.3f });
}
