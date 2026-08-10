#pragma once

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
	Shot(AssetStore& assets, World& world, sf::Texture& texture,
		const GameplayData::ProjectileConfig& config);

	void Update(float deltaTime) override;
	[[nodiscard]] int GetDamage() const noexcept;
	[[nodiscard]] float GetKnockback() const noexcept;

protected:
	void SetDirection(const sf::Vector2f& direction) noexcept;

private:
	float speed{ 0.f };
	int damage{ 0 };
	float knockback{ 0.f };
};

class PlayerShot final : public Shot
{
public:
	PlayerShot(AssetStore& assets, World& world, const sf::Vector2f& position, float rotationDegrees);
	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
};

class SaucerShot final : public Shot
{
public:
	SaucerShot(AssetStore& assets, World& world, const sf::Vector2f& position,
		const sf::Vector2f& targetPosition, int currentScore);
	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
};
