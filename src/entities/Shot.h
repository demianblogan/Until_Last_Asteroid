#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>

#include "core/Entity.h"
#include "gameplay/GameplayData.h"

class World;
class Assets;

namespace sf
{
	class Texture;
}

class Shot : public Entity
{
public:
	enum class VisualKind { Player, PlayerHoming, PlayerTriple, Helper, Enemy };

	Shot(Assets& assets, World& world, sf::Texture& texture,
		const GameplayData::ProjectileConfig& config, VisualKind visualKind,
		std::uint64_t playerAttackID = 0u);

	void Update(float deltaTime) override;
	[[nodiscard]] int GetDamage() const noexcept;
	[[nodiscard]] float GetKnockback() const noexcept;
	[[nodiscard]] std::uint64_t GetPlayerAttackID() const noexcept;
	void ReflectToward(const sf::Vector2f& targetPosition);

protected:
	void SetDirection(const sf::Vector2f& direction) noexcept;
	[[nodiscard]] bool IsReflected() const noexcept;

private:
	float speed{ 0.f };
	int damage{ 0 };
	float knockback{ 0.f };
	VisualKind visualKind;
	std::uint64_t playerAttackID{ 0u };
	bool reflected{ false };
};

class PlayerShot final : public Shot
{
public:
	PlayerShot(Assets& assets, World& world, const sf::Vector2f& position,
		float rotationDegrees, std::uint64_t attackID, bool playSound = true,
		bool tripleShotVisual = false);
	void Update(float deltaTime) override;
	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;

private:
	void AcquireHomingTarget();
	void UpdateHoming(float deltaTime);

	const Entity* homingTarget{ nullptr };
	std::optional<std::size_t> bossHomingTarget;
	bool homingEnabled{ false };
};

class SaucerShot final : public Shot
{
public:
	SaucerShot(Assets& assets, World& world, const sf::Vector2f& position,
		const sf::Vector2f& targetPosition,
		GameplayData::ProjectileKind projectileKind = GameplayData::ProjectileKind::Enemy,
		bool playSound = true);
	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
};

class HelperShot final : public Shot
{
public:
	HelperShot(Assets& assets, World& world, const sf::Vector2f& position,
		const Entity* target);
	void Update(float deltaTime) override;
	[[nodiscard]] Type GetType() const noexcept override;
	[[nodiscard]] bool IsCollideWith(const Entity& other) const override;

private:
	void AcquireTarget();
	void UpdateHoming(float deltaTime);

	const Entity* homingTarget{ nullptr };
};
