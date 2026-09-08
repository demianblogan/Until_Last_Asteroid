#pragma once

#include <string>

#include "core/Entity.h"

// A ship-upgrade part dropped by a specific enemy (see Enemy::partDropID).
// Sits in place, pulsing gently, and despawns on its own if the player
// doesn't collect it in time (blinking faster as remainingLifetime runs
// out) -- unlike Pickup, there's no explicit Apply(); collecting it is
// handled entirely by World noticing the collision and recording the ID.
class Part final : public Entity
{
public:
	Part(Assets& assets, World& world, std::string id);

	[[nodiscard]] const std::string& GetID() const noexcept;
	Type GetType() const noexcept override;

private:
	void Update(float deltaTime) override;
	bool IsCollidingWith(const Entity& other) const override;

	std::string id;
	float remainingLifetime = 3.f;

	// Counts up (never resets), unlike remainingLifetime which counts down --
	// purely the "clock" driving the gentle pulsing-size visual below.
	float pulsePhaseElapsed = 0.f;
};