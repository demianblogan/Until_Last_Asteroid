#pragma once

#include "Enemy.h"

class AssetStore;
class World;

class MissileCarrier final : public Enemy
{
public:
	MissileCarrier(AssetStore& assets, World& world);

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	void LaunchMissile(const sf::Vector2f& target);
	void EmitEngineParticles(const sf::Vector2f& exhaustDirection);
	[[nodiscard]] sf::Vector2f GetLauncherPosition() const;
	[[nodiscard]] sf::Vector2f GetEmitterPosition(
		const GameplayData::NormalizedPoint& emitter) const;

	float launchTimer{ 0.f };
};
