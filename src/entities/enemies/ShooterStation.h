#pragma once

#include <cstdint>

#include "Enemy.h"

// A stationary(-ish) installation that rides a fixed path between two points
// like LaserTurret, but instead of firing itself, periodically spawns a
// ShooterSaucer to guard it (see World::SpawnStationShooter). Immune to
// damage while its shield is up -- shown by a "welding" charge-up animation
// each time it re-arms the shield (see GetSpawnChargeRatio) -- and immune to
// knockback entirely, so a hit can't shove it off its path. Destruction is a
// multi-stage sequence (see BeginDestruction/isDestructionActive): it shakes
// and throws off a series of small explosions for about a second, ignoring
// further collisions the whole time, before actually being removed.
class ShooterStation final : public Enemy
{
public:
	ShooterStation(Assets& assets, World& world);

	void ConfigurePath(sf::Vector2f first, sf::Vector2f second);
	void ConfigureStationaryArrival(sf::Vector2f start, sf::Vector2f destination);

	[[nodiscard]] bool IsShieldActive() const noexcept;
	[[nodiscard]] float GetShieldPulse() const noexcept;
	[[nodiscard]] float GetShieldRadius() const noexcept;
	[[nodiscard]] bool CollidesWithPlayerProjectile(const Entity& projectile) const override;
	[[nodiscard]] sf::Vector2f GetPlayerProjectileImpactPosition(const Entity& projectile) const noexcept override;

	[[nodiscard]] float GetSpawnChargeRatio() const noexcept;
	[[nodiscard]] bool IsArriving() const noexcept override;
	[[nodiscard]] bool TakeDamage(int damage) override;
	[[nodiscard]] bool AcceptsKnockback() const noexcept override;

	Type GetType() const noexcept override;
	bool IsCollidingWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	void BeginDestruction() override;
	[[nodiscard]] bool ReachedTarget() const noexcept;

	sf::Vector2f pathStart;
	sf::Vector2f pathEnd;
	sf::Vector2f targetPoint;
	bool isPathConfigured = false;
	bool isArriving = false;

	float spawnElapsed = 0.f;

	// How often this station spawns a new ShooterSaucer to guard it (see
	// World::SpawnStationShooter), once its own creation/shield animation
	// has finished.
	float spawnInterval = 0.f;
	float shieldDuration = 3.f;
	float spawnAnimationDuration = 3.f;
	float creationRemaining = 0.f;
	float weldingAccumulator = 0.f;
	std::uint64_t workingSoundHandle = 0u;

	sf::Vector2f destructionOrigin;
	float destructionRemaining = 0.f;
	float destructionExplosionAccumulator = 0.f;
	float destructionElapsed = 0.f;
	bool isDestructionActive = false;
};
