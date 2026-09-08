#pragma once

#include "core/Entity.h"

// A player companion drone, spawned by the HelperBot pickup bonus. Orbits
// the player at a fixed radius (see orbitPhaseRadians), and periodically
// fires a homing HelperShot at the nearest enemy in front of it if one's in
// range. Never collides with anything itself (see IsCollidingWith) and
// self-destructs the moment the bonus expires or the player is gone.
class HelperBot final : public Entity
{
public:
	HelperBot(Assets& assets, World& world);

	[[nodiscard]] Type GetType() const noexcept override;
	[[nodiscard]] bool IsCollidingWith(const Entity&) const override;
	void Update(float deltaTime) override;

private:
	float orbitPhaseRadians = 0.f;
	float shotCooldown = 0.f;
};
