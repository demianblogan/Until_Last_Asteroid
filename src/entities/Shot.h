#pragma once

#include "core/Entity.h"

class World;
class AssetStore;

namespace sf
{
	class Texture;
}

class Shot : public Entity
{
public:
	Shot(AssetStore& assets, World& world, sf::Texture& texture, float speed) noexcept;

	void Update(float deltaTime) override;

protected:
	void SetDirection(const sf::Vector2f& direction) noexcept;

private:
	float speed{ 0.f };
};

class PlayerShot final : public Shot
{
public:
	PlayerShot(AssetStore& assets, World& world, const sf::Vector2f& position, float rotationDegrees);

	virtual Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;

	static constexpr float SPEED = 1000.f;
};

class SaucerShot final : public Shot
{
public:
	SaucerShot(AssetStore& assets, World& world, const sf::Vector2f& position, const sf::Vector2f& targetPosition, int currentScore);

	virtual Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;

	static constexpr float SPEED = 750.f;
};