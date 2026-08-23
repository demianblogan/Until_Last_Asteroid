#pragma once

#include <string>

#include "core/Entity.h"

class Part final : public Entity
{
public:
	Part(Assets& assets, World& world, std::string id);

	[[nodiscard]] const std::string& GetID() const noexcept;
	Type GetType() const noexcept override;

private:
	void Update(float deltaTime) override;
	bool IsCollideWith(const Entity& other) const override;

	std::string id;
	float remainingLifetime{ 3.f };
	float elapsed{ 0.f };
};
