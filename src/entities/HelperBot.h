#pragma once

#include "core/Entity.h"

class HelperBot final : public Entity
{
public:
	HelperBot(AssetStore& assets, World& world);

	[[nodiscard]] Type GetType() const noexcept override;
	[[nodiscard]] bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;

private:
	float orbitPhaseRadians{ 0.f };
	float shotCooldown{ 0.f };
};
