#pragma once

#include <cstdint>

#include "Enemy.h"

// Rides a fixed rail between two corners, alternating between traveling and
// pausing at each end (see the Arriving/Waiting/Traversing Phase enum
// below). Doesn't shoot projectiles at all -- while Traversing, it instead
// fires a continuous, full-screen-width laser beam perpendicular to its
// travel direction (see GetBeamStart/GetBeamEnd), which damages the player
// every frame it's active (see World::DamagePlayerWithBeam). Doesn't accept
// knockback, since a hit shouldn't be able to knock it off its rail.
class LaserTurret final : public Enemy
{
public:
	LaserTurret(Assets& assets, World& world);

	void ConfigurePath(sf::Vector2f first, sf::Vector2f second, sf::Vector2f inward);
	void ConfigureStationaryArrival(sf::Vector2f start, sf::Vector2f destination, sf::Vector2f beamDirection);

	[[nodiscard]] sf::Vector2f GetBeamStart() const noexcept;
	[[nodiscard]] sf::Vector2f GetBeamEnd() const noexcept;
	[[nodiscard]] float GetBeamWidth() const noexcept;
	[[nodiscard]] float GetBeamPulse() const noexcept;
	[[nodiscard]] float GetBeamAnimationTime() const noexcept;
	[[nodiscard]] bool IsBeamActive() const noexcept;
	[[nodiscard]] bool IsArriving() const noexcept override;
	[[nodiscard]] bool AcceptsKnockback() const noexcept override;

	Type GetType() const noexcept override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	enum class Phase
	{
		Arriving,
		Waiting,
		Traversing
	};

	void BeginTraversal();
	[[nodiscard]] bool ReachedTarget(sf::Vector2f target) const noexcept;

	sf::Vector2f pathStart;
	sf::Vector2f pathEnd;
	sf::Vector2f targetCorner;
	sf::Vector2f traversalOrigin;
	sf::Vector2f inward{ 0.f, 1.f };

	float beamWidth = 18.f;
	float beamTime = 0.f;
	float laserDuration = 1.f;
	float traversalElapsed = 0.f;
	float waitRemaining = 0.f;
	int beamDamage = 20;

	std::uint64_t laserSoundHandle = 0u;
	Phase phase = Phase::Arriving;
	bool isAtFirstCorner = false;
};