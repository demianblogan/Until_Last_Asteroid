#pragma once

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>
#include "systems/Collision.h"

namespace sf
{
	class Texture;
	class RenderTarget;
}

class World;
class AssetStore;

class Entity : public sf::Drawable
{
public:
	enum class Type
	{
		Player,
		Enemy,
		Projectile_Player,
		Projectile_Enemy,
		Asteroid
	};

	Entity(AssetStore& assets, World& world, sf::Texture& texture) noexcept;

	Entity(const Entity&) = delete;
	Entity& operator=(const Entity&) = delete;

	Entity(Entity&&) = default;
	Entity& operator=(Entity&&) = default;

	virtual ~Entity() = default;

	void SetPosition(const sf::Vector2f& position) noexcept;
	[[nodiscard]] sf::Vector2f GetPosition() const noexcept;

	void SetVelocity(const sf::Vector2f& velocity) noexcept;
	[[nodiscard]] const sf::Vector2f& GetVelocity() const noexcept;

	[[nodiscard]] bool IsAlive() const noexcept;
	void Destroy() noexcept;

	[[nodiscard]] const sf::Sprite& GetSprite() const noexcept;
	virtual Type GetType() const noexcept = 0;

protected:
	[[nodiscard]] World& GetWorld() noexcept;
	[[nodiscard]] AssetStore& GetAssets() noexcept;

	void SetRotation(sf::Angle angle) noexcept;
	[[nodiscard]] sf::Angle GetRotation() const noexcept;

	virtual void Update(float deltaTime) = 0;
	virtual bool IsCollideWith(const Entity& other) const = 0;
	[[nodiscard]] bool CheckCollision(const Entity& other) const noexcept;

	void Move(float deltaTime) noexcept;
	void Accelerate(const sf::Vector2f& delta) noexcept;

	virtual void OnDestroy();

private:
	sf::Sprite sprite;
	sf::Vector2f velocity{ 0.f, 0.f };

	AssetStore& assets;
	World& world;

	bool isAlive{ true };

private:
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	friend class World;
};