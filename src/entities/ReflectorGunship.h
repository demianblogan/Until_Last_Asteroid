#pragma once

#include "Enemy.h"

class ReflectorGunship final : public Enemy
{
public:
	ReflectorGunship(Assets& assets, World& world);

	void ConfigureApproachTarget(sf::Vector2f target) noexcept;
	[[nodiscard]] bool IsShieldActive() const noexcept;
	[[nodiscard]] float GetShieldPulse() const noexcept;
	[[nodiscard]] float GetShieldRadius() const noexcept;
	[[nodiscard]] bool IsProjectileReflectionActive() const noexcept override;
	[[nodiscard]] bool BlocksPlayerLaser() const noexcept override;
	[[nodiscard]] bool AcceptsKnockback() const noexcept override;
	[[nodiscard]] bool CollidesWithPlayerProjectile(
		const Entity& projectile) const override;
	[[nodiscard]] sf::Vector2f GetPlayerProjectileImpactPosition(
		const Entity& projectile) const noexcept override;
	[[nodiscard]] Type GetType() const noexcept override;
	[[nodiscard]] bool IsCollidingWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	[[nodiscard]] bool MoveToApproachTarget(float deltaTime);
	void UpdateFigureEight(float deltaTime);
	void ShootDoubleVolley(const sf::Vector2f& playerPosition);
	[[nodiscard]] sf::Vector2f GetWeaponEmitterPosition(std::size_t index) const;

	sf::Vector2f approachTarget;
	sf::Vector2f figureEightCenter;
	float movementPhase{ 0.f };
	float shootTimer{ 0.f };
	float shootInterval{ 0.5f };
	float shieldPhaseElapsed{ 0.f };
	float shieldDuration{ 3.f };
	float figureEightAmplitude{ 350.f };
	float figureEightFrequency{ 1.1f };
	bool approachingCenter{ false };
	bool shieldActive{ false };
};
