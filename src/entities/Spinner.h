#pragma once

#include "Enemy.h"

class Assets;
class World;

class Spinner final : public Enemy
{
public:
	Spinner(Assets& assets, World& world);
	void ConfigureApproachTarget(sf::Vector2f target) noexcept;

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	void ShootRadialVolley();
	[[nodiscard]] sf::Vector2f GetWeaponEmitterPosition(std::size_t index) const;

	sf::Vector2f travelDirection;
	sf::Vector2f lateralDirection;
	float sineAmplitude{ 0.f };
	float sineFrequency{ 0.f };
	float movementPhase{ 0.f };
	float shootTimer{ 0.f };
	float spinDirection{ 1.f };
	sf::Vector2f approachTarget;
	bool approachingCenter{ false };
};
