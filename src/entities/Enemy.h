#pragma once

#include "core/Entity.h"
#include "gameplay/GameplayData.h"
#include "gameplay/Health.h"

#include <string>

class Assets;
class World;

namespace sf
{
	class Texture;
}

class Enemy : public Entity
{
public:
	Enemy(Assets& assets, World& world, sf::Texture& texture,
		const GameplayData::EnemyConfig& config);

	[[nodiscard]] int GetScoreValue() const noexcept;
	[[nodiscard]] int GetContactDamage() const noexcept;
	[[nodiscard]] float GetCollisionImpulse() const noexcept;
	[[nodiscard]] float GetSoundPitch() const noexcept;
	[[nodiscard]] int GetCurrentHealth() const noexcept;
	void SetPickupDrop(
		const std::optional<GameplayData::SpawnGroup::PickupDropConfig>& drop);
	[[nodiscard]] std::optional<GameplayData::PickupKind> RollPickupDrop() const;
	void SetOrderedPickupDropCount(int count) noexcept;
	[[nodiscard]] int GetOrderedPickupDropCount() const noexcept;
	void SetPartDropID(std::string id);
	[[nodiscard]] const std::string& GetPartDropID() const noexcept;
	void SetRewardsEnabled(bool enabled) noexcept;
	[[nodiscard]] bool AreRewardsEnabled() const noexcept;
	void SetScoreRewardEnabled(bool enabled) noexcept;
	void SetPickupRewardsEnabled(bool enabled) noexcept;
	void SetPartRewardEnabled(bool enabled) noexcept;
	[[nodiscard]] bool IsScoreRewardEnabled() const noexcept;
	[[nodiscard]] bool ArePickupRewardsEnabled() const noexcept;
	[[nodiscard]] bool IsPartRewardEnabled() const noexcept;
	[[nodiscard]] virtual bool AcceptsKnockback() const noexcept;
	[[nodiscard]] virtual bool IsProjectileReflectionActive() const noexcept;
	[[nodiscard]] virtual bool BlocksPlayerLaser() const noexcept;
	[[nodiscard]] virtual bool CollidesWithPlayerProjectile(
		const Entity& projectile) const;
	[[nodiscard]] virtual sf::Vector2f GetPlayerProjectileImpactPosition(
		const Entity& projectile) const noexcept;
	[[nodiscard]] virtual bool TakeDamage(int damage);
	Type GetType() const noexcept override;

protected:
	void Update(float deltaTime) override;
	[[nodiscard]] float GetMovementSpeed() const noexcept;
	[[nodiscard]] float GetActionInterval() const noexcept;
	[[nodiscard]] float GetFragmentSpeed() const noexcept;
	[[nodiscard]] float GetRotationSpeed() const noexcept;
	[[nodiscard]] const std::vector<GameplayData::NormalizedPoint>&
		GetWeaponEmitters() const noexcept;
	[[nodiscard]] const std::vector<GameplayData::NormalizedPoint>&
		GetEngineEmitters() const noexcept;
	virtual void BeginDestruction();

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
	std::vector<GameplayData::NormalizedPoint> weaponEmitters;
	std::vector<GameplayData::NormalizedPoint> engineEmitters;
	std::optional<GameplayData::SpawnGroup::PickupDropConfig> pickupDrop;
	int orderedPickupDropCount{ 0 };
	std::string partDropID;
	bool scoreRewardEnabled{ true };
	bool pickupRewardsEnabled{ true };
	bool partRewardEnabled{ true };
};
