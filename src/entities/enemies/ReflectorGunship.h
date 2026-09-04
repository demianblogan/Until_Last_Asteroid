#pragma once

#include "Enemy.h"

// Flies a figure-eight pattern around a fixed center, firing a two-shot
// volley on a timer. Periodically toggles a shield on and off (see
// isShieldActive/shieldPhaseElapsed): while it's up, the player's own shots
// are reflected straight back at them instead of dealing damage (see
// IsProjectileReflectionActive), and the player's laser passes through
// without hurting it (BlocksPlayerLaser) -- both routed through the shield's
// own collision circle (GetShieldRadius), not the ship's regular hitbox.
// Never accepts knockback, so a hit (or a reflected shot bouncing off the
// shield) can't shove it off its pattern.
class ReflectorGunship final : public Enemy
{
public:
	ReflectorGunship(Assets& assets, World& world);

	void ConfigureApproachTarget(sf::Vector2f target) noexcept override;

	[[nodiscard]] bool IsShieldActive() const noexcept;
	[[nodiscard]] float GetShieldPulse() const noexcept;
	[[nodiscard]] float GetShieldRadius() const noexcept;

	[[nodiscard]] bool IsProjectileReflectionActive() const noexcept override;
	[[nodiscard]] bool BlocksPlayerLaser() const noexcept override;
	[[nodiscard]] bool AcceptsKnockback() const noexcept override;
	[[nodiscard]] bool CollidesWithPlayerProjectile(const Entity& projectile) const override;
	[[nodiscard]] sf::Vector2f GetPlayerProjectileImpactPosition(const Entity& projectile) const noexcept override;

	[[nodiscard]] bool IsCollidingWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	void UpdateFigureEight(float deltaTime);

	void ShootDoubleVolley();

	sf::Vector2f figureEightCenter;
	float movementPhase = 0.f;
	float figureEightAmplitude = 350.f;
	float figureEightFrequency = 1.1f;

	float shootTimer = 0.f;
	float shootInterval = 0.5f;

	float shieldPhaseElapsed = 0.f;
	float shieldDuration = 3.f;
	bool isShieldActive = false;
};
