#pragma once

#include <cstdint>

#include "core/Entity.h"
#include "game/GameplayData.h"

class World;
class AssetStore;

namespace sf
{
	class Texture;
}

class Shot : public Entity
{
public:
	enum class VisualKind { Player, PlayerHoming, PlayerTriple, Enemy };

	Shot(AssetStore& assets, World& world, sf::Texture& texture,
		const GameplayData::ProjectileConfig& config, VisualKind visualKind,
		std::uint64_t playerAttackId = 0u);

	void Update(float deltaTime) override;
	[[nodiscard]] int GetDamage() const noexcept;
	[[nodiscard]] float GetKnockback() const noexcept;
	[[nodiscard]] std::uint64_t GetPlayerAttackId() const noexcept;

protected:
	void SetDirection(const sf::Vector2f& direction) noexcept;

private:
	float speed{ 0.f };
	int damage{ 0 };
	float knockback{ 0.f };
	VisualKind visualKind;
	std::uint64_t playerAttackId{ 0u };
};

class PlayerShot final : public Shot
{
public:
	PlayerShot(AssetStore& assets, World& world, const sf::Vector2f& position,
		float rotationDegrees, std::uint64_t attackId, bool playSound = true,
		bool tripleShotVisual = false);
	void Update(float deltaTime) override;
	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;

private:
	void AcquireHomingTarget();
	void UpdateHoming(float deltaTime);

	const Entity* homingTarget{ nullptr };
	bool homingEnabled{ false };
};

class SaucerShot final : public Shot
{
public:
	SaucerShot(AssetStore& assets, World& world, const sf::Vector2f& position,
		const sf::Vector2f& targetPosition,
		GameplayData::ProjectileKind projectileKind = GameplayData::ProjectileKind::Enemy,
		bool playSound = true);
	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
};
