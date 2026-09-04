#pragma once

#include "Enemy.h"

class Assets;
class World;

// Flies to a central patrol point (see ConfigureApproachTarget), then
// wanders between random patrol points while firing a periodic single shot
// at the player. Also the enemy a ShooterStation spawns to guard itself,
// hence the materialization ("beam in") effect -- nothing else uses it.
class ShooterSaucer final : public Enemy
{
public:
	ShooterSaucer(Assets& assets, World& world);

	// Fades this saucer in at its current position over `duration` seconds,
	// tracking `anchor`'s position while it does -- used when a
	// ShooterStation spawns one as a guard, so it visibly emerges from the
	// station rather than just appearing.
	void BeginMaterialization(float duration, const Entity* anchor = nullptr) noexcept;

	void ConfigureApproachTarget(sf::Vector2f target) noexcept override;
	void OnDestroy() override;

private:
	void Update(float deltaTime) override;

	void ChooseCentralPatrolTarget();
	void Shoot();

	sf::Vector2f patrolTarget;
	bool hasPatrolTarget = false;

	float shootTimer = 0.f;
	float shootInterval = 0.f;
	std::size_t nextWeaponEmitter = 0;

	float materializationRemaining = 0.f;
	float materializationDuration = 0.f;
	const Entity* materializationAnchor = nullptr;
};
