#pragma once

#include "core/Entity.h"
#include "game/Health.h"

class AssetStore;
class World;

class HomingMissile final : public Entity
{
public:
	HomingMissile(
		AssetStore& assets,
		World& world,
		const sf::Vector2f& position,
		const sf::Vector2f& target);

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

	[[nodiscard]] bool TakeDamage(int damage);
	void Detonate();

private:
	Health health;
	float speed{ 0.f };
	float turnSpeedRadians{ 0.f };
	float explosionRadius{ 0.f };
	float explosionImpulse{ 0.f };
	float lifetimeRemaining{ 0.f };
	int explosionDamage{ 0 };
	bool detonating{ false };
};
