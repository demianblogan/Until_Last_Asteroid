#pragma once

#include "core/Entity.h"
#include "game/GameplayData.h"
#include "game/Health.h"

class AssetStore;
class World;

namespace sf
{
	class Texture;
}

class Enemy : public Entity
{
public:
	Enemy(AssetStore& assets, World& world, sf::Texture& texture,
		const GameplayData::EnemyConfig& config);

	[[nodiscard]] int GetScoreValue() const noexcept;
	[[nodiscard]] int GetContactDamage() const noexcept;
	[[nodiscard]] float GetCollisionImpulse() const noexcept;
	[[nodiscard]] float GetSoundPitch() const noexcept;
	[[nodiscard]] int GetCurrentHealth() const noexcept;
	[[nodiscard]] bool TakeDamage(int damage);
	Type GetType() const noexcept override;

protected:
	void Update(float deltaTime) override;
	[[nodiscard]] float GetMovementSpeed() const noexcept;
	[[nodiscard]] float GetActionInterval() const noexcept;
	[[nodiscard]] float GetFragmentSpeed() const noexcept;
	[[nodiscard]] float GetRotationSpeed() const noexcept;
	[[nodiscard]] const std::array<GameplayData::NormalizedPoint, 2>&
		GetWeaponEmitters() const noexcept;

private:
	Health health;
	int scoreValue{ 0 };
	int contactDamage{ 0 };
	float movementSpeed{ 0.f };
	float collisionImpulse{ 0.f };
	float actionInterval{ 0.f };
	float fragmentSpeed{ 0.f };
	float soundPitch{ 1.f };
	float rotationSpeed{ 0.f };
	std::array<GameplayData::NormalizedPoint, 2> weaponEmitters{};
};
