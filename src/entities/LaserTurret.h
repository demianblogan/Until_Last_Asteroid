#pragma once

#include <cstdint>

#include "Enemy.h"

class LaserTurret final : public Enemy
{
public:
	LaserTurret(AssetStore& assets, World& world);

	void ConfigurePath(sf::Vector2f first, sf::Vector2f second, sf::Vector2f inward);
	[[nodiscard]] sf::Vector2f GetBeamStart() const noexcept;
	[[nodiscard]] sf::Vector2f GetBeamEnd() const noexcept;
	[[nodiscard]] float GetBeamWidth() const noexcept;
	[[nodiscard]] float GetBeamPulse() const noexcept;
	[[nodiscard]] float GetBeamAnimationTime() const noexcept;
	[[nodiscard]] bool IsBeamActive() const noexcept;
	[[nodiscard]] bool IsArriving() const noexcept;
	[[nodiscard]] bool AcceptsKnockback() const noexcept override;

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	enum class Phase
	{
		Arriving,
		Waiting,
		Traversing
	};

	void BeginTraversal();
	[[nodiscard]] bool ReachedTarget(sf::Vector2f target) const noexcept;

	sf::Vector2f pathStart;
	sf::Vector2f pathEnd;
	sf::Vector2f targetCorner;
	sf::Vector2f traversalOrigin;
	sf::Vector2f inward{ 0.f, 1.f };
	float beamWidth{ 18.f };
	float beamTime{ 0.f };
	float laserDuration{ 1.f };
	float traversalElapsed{ 0.f };
	float waitRemaining{ 0.f };
	int beamDamage{ 20 };
	std::uint64_t laserSoundHandle{ 0u };
	Phase phase{ Phase::Arriving };
	bool atFirstCorner{ false };
};
