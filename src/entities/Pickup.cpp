#include "Pickup.h"

#include <algorithm>
#include <cmath>

#include "assets/AssetStore.h"
#include "game/GameplaySession.h"

namespace
{
    Config::Texture GetTexture(Pickup::Kind kind)
    {
		switch (kind)
		{
		case Pickup::Kind::Health:
			return Config::Texture::HealthPickup;
		case Pickup::Kind::Shield:
			return Config::Texture::ShieldPickup;
		case Pickup::Kind::HomingBullets:
			return Config::Texture::HomingBulletsPickup;
		case Pickup::Kind::TimeSlowdown:
			return Config::Texture::TimeSlowdownPickup;
		}
		return Config::Texture::HomingBulletsPickup;
    }
}

Pickup::Pickup(AssetStore& assets, World& world, Kind pickupKind)
    : Entity(
        assets,
        world,
        assets.Textures().Get(GetTexture(pickupKind)),
        assets.GetGameplayData().GetPickups().visualScale,
        assets.GetGameplayData().GetPickups().collisionRadius)
    , kind(pickupKind)
    , config(assets.GetGameplayData().GetPickups())
{
}

Entity::Type Pickup::GetType() const noexcept
{
    return Type::Pickup;
}

bool Pickup::IsCollideWith(const Entity& other) const
{
    return other.GetType() == Type::Player && CheckCollision(other);
}

void Pickup::Update(float deltaTime)
{
    static_cast<void>(deltaTime);
}

bool Pickup::Apply(GameplaySession& session)
{
    if (kind == Kind::Shield)
    {
        session.ActivateShield();
        return true;
    }
	if (kind == Kind::HomingBullets)
	{
		session.ActivateHomingBullets(config.homingBulletsDuration);
		return true;
	}
	if (kind == Kind::TimeSlowdown)
	{
		session.ActivateTimeSlowdown(config.timeSlowdownDuration);
		return true;
	}

    const int maximumHealth{ session.GetPlayerHealth().GetMaximum() };
    const int amount{ std::max(1, static_cast<int>(
        std::round(maximumHealth * config.healthRestorePercentage))) };
    return session.RestorePlayerHealth(amount);
}

Pickup::Kind Pickup::GetKind() const noexcept
{
    return kind;
}
