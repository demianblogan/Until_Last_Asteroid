#pragma once

#include <cstdint>

#include "Enemy.h"

class ShooterStation final : public Enemy
{
public:
	ShooterStation(Assets& assets, World& world);
	void ConfigurePath(sf::Vector2f first, sf::Vector2f second);
	void ConfigureStationaryArrival(sf::Vector2f start, sf::Vector2f destination);

	[[nodiscard]] bool IsShieldActive() const noexcept;
	[[nodiscard]] float GetShieldPulse() const noexcept;
	[[nodiscard]] float GetSpawnChargeRatio() const noexcept;
	[[nodiscard]] bool IsArriving() const noexcept override;
	[[nodiscard]] bool TakeDamage(int damage) override;
	[[nodiscard]] bool AcceptsKnockback() const noexcept override;
	[[nodiscard]] bool CollidesWithPlayerProjectile(
		const Entity& projectile) const override;
	[[nodiscard]] sf::Vector2f GetPlayerProjectileImpactPosition(
		const Entity& projectile) const noexcept override;
	[[nodiscard]] float GetShieldRadius() const noexcept;
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
	sf::Vector2f destructionOrigin;
	float spawnElapsed{ 0.f };
	float shieldDuration{ 3.f };
	float spawnAnimationDuration{ 3.f };
	float creationRemaining{ 0.f };
	float weldingAccumulator{ 0.f };
	float destructionRemaining{ 0.f };
	float destructionExplosionAccumulator{ 0.f };
	float destructionElapsed{ 0.f };
	std::uint64_t workingSoundHandle{ 0u };
	bool pathConfigured{ false };
	bool arriving{ false };
	bool destructionActive{ false };
};
