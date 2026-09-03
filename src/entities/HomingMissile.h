#pragma once

#include "core/Entity.h"
#include "gameplay/Health.h"

class Assets;
class World;

// A guided missile fired by MissileCarrier. Unlike Shot, it has its own
// health and can be shot down by the player before it arrives (see
// TakeDamage); reaching the player (or running out of health) detonates it
// for area-of-effect damage and knockback around the blast (see Detonate/
// explosionRadius/explosionImpulse) rather than a single point hit.
// Gradually turns toward the player each frame, same as a homing PlayerShot.
class HomingMissile final : public Entity
{
public:
	HomingMissile(Assets& assets, World& world, const sf::Vector2f& position, const sf::Vector2f& target);

	Type GetType() const noexcept override;
	bool IsCollidingWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

	[[nodiscard]] bool TakeDamage(int damage);
	void Detonate();

private:
	Health health;
	float speed = 0.f;
	float turnSpeedRadians = 0.f;
	float explosionRadius = 0.f;
	float explosionImpulse = 0.f;
	float lifetimeRemaining = 0.f;
	int explosionDamage = 0;
	bool isDetonating = false;
};