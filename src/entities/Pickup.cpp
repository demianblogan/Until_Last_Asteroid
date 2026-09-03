#include "Pickup.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "assets/Assets.h"
#include "gameplay/GameplaySession.h"

namespace
{
	Config::Texture GetTexture(Pickup::Kind kind)
	{
		switch (kind)
		{
			using enum Pickup::Kind;

		case Health:
			return Config::Texture::HealthPickup;
		case Shield:
			return Config::Texture::ShieldPickup;
		case HomingBullets:
			return Config::Texture::HomingBulletsPickup;
		case TimeSlowdown:
			return Config::Texture::TimeSlowdownPickup;
		case Laser:
			return Config::Texture::LaserPickup;
		case TripleShot:
			return Config::Texture::TripleShotPickup;
		case HelperBot:
			return Config::Texture::HelperBotPickup;

		default:
			std::unreachable();
		}
	}
}

Pickup::Pickup(Assets& assets, World& world, Kind pickupKind)
	: Entity(assets, world,
		assets.Textures().Get(GetTexture(pickupKind)),
		assets.GetGameplayData().GetPickups().visualScale,
		assets.GetGameplayData().GetPickups().collisionRadius)
	, kind(pickupKind)
	, config(assets.GetGameplayData().GetPickups())
{}

Entity::Type Pickup::GetType() const noexcept
{
	return Type::Pickup;
}

bool Pickup::IsCollidingWith(const Entity& other) const
{
	return other.GetType() == Type::Player && CheckCollision(other);
}

void Pickup::Update(float)
{
	// A pickup is stationary and doesn't animate on its own -- nothing to do
	// each frame. Kept as an override (rather than left out entirely) only
	// because Entity::Update is pure virtual.
}

bool Pickup::Apply(GameplaySession& session)
{
	const float bonusDuration = session.GetBonusDurationAddition();

	switch (kind)
	{
		using enum Kind;

	case Shield:
		session.ActivateShield(bonusDuration);
		return true;

	case HomingBullets:
		session.ActivateHomingBullets(config.homingBulletsDuration + bonusDuration);
		return true;

	case TimeSlowdown:
		session.ActivateTimeSlowdown(config.timeSlowdownDuration + bonusDuration);
		return true;

	case Laser:
		session.ActivateLaser(config.laserDuration + bonusDuration);
		return true;

	case TripleShot:
		session.ActivateTripleShot(config.tripleShotDuration + bonusDuration);
		return true;

	case HelperBot:
		return session.ActivateHelperBot();

	case Health:
	{
		const int maximumHealth = session.GetPlayerHealth().GetMaximum();
		const int amount = std::max(1, static_cast<int>(std::round(maximumHealth * config.healthRestorePercentage)));
		return session.RestorePlayerHealth(amount);
	}

	default:
		std::unreachable();
	}
}

Pickup::Kind Pickup::GetKind() const noexcept
{
	return kind;
}