#pragma once

#include "Enemy.h"

class Assets;
class World;

// Always faces the player (see TurnTowards in Update()) while wandering
// between random points near the screen's center, trailing engine particles.
// Periodically launches a HomingMissile at the player rather than shooting
// directly itself.
class MissileCarrier final : public Enemy
{
public:
	MissileCarrier(Assets& assets, World& world);

	void ConfigureApproachTarget(sf::Vector2f target) noexcept override;

	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	void ChooseCentralPatrolTarget();
	void UpdatePatrolMovement(float deltaTime);

	void LaunchMissile(const sf::Vector2f& target);
	[[nodiscard]] sf::Vector2f GetLauncherPosition() const;

	void EmitEngineParticles(const sf::Vector2f& exhaustDirection);

	float launchTimer = 0.f;
	float launchInterval = 0.f;

	sf::Vector2f patrolTarget;
	bool hasPatrolTarget = false;
};
