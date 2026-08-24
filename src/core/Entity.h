#pragma once

#include <span>
#include <vector>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/Collision.h"

namespace sf
{
	class Texture;
	class RenderTarget;
	class Shader;
}

class World;
class Assets;

class Entity : public sf::Drawable
{
public:
	enum class Type
	{
		Player,
		Companion,
		Enemy,
		Projectile_Player,
		Projectile_Ally,
		Projectile_Enemy,
		EnemyMissile,
		Asteroid,
		Pickup,
		Part
	};

	Entity(Assets& assets, World& world, sf::Texture& texture,
		float visualScale, float collisionRadius,
		std::span<const Collision::LocalCircle> collisionCircles = {});

	virtual ~Entity() = default;

	Entity(const Entity&) = delete;
	Entity& operator=(const Entity&) = delete;

	Entity(Entity&&) = default;
	Entity& operator=(Entity&&) = default;

	void SetPosition(const sf::Vector2f& position) noexcept;
	[[nodiscard]] sf::Vector2f GetPosition() const noexcept;

	void SetVelocity(const sf::Vector2f& velocity) noexcept;
	[[nodiscard]] const sf::Vector2f& GetVelocity() const noexcept;

	void ApplyImpulse(const sf::Vector2f& impulse) noexcept;
	void Translate(const sf::Vector2f& offset) noexcept;

	void SetPresentation(float scaleMultiplier, float opacity, sf::Color tint = sf::Color::White) noexcept;

	[[nodiscard]] bool IsAlive() const noexcept;
	void Destroy() noexcept;

	[[nodiscard]] const sf::Sprite& GetSprite() const noexcept;
	[[nodiscard]] float GetCollisionRadius() const noexcept;
	virtual Type GetType() const noexcept = 0;

	// Overridden by entities (ShooterStation, LaserTurret) that stay outside
	// normal wrap-around behavior while materializing. Defaulting to false
	// here avoids a dynamic_cast check against every entity, every frame, in
	// World::Update just to test this for the rare entities that care.
	[[nodiscard]] virtual bool IsArriving() const noexcept { return false; }

protected:
	[[nodiscard]] World& GetWorld() noexcept;
	[[nodiscard]] const World& GetWorld() const noexcept;

	[[nodiscard]] Assets& GetAssets() noexcept;
	[[nodiscard]] const Assets& GetAssets() const noexcept;

	void SetRotation(sf::Angle angle) noexcept;
	[[nodiscard]] sf::Angle GetRotation() const noexcept;

	void TurnTowards(const sf::Vector2f& target, float maximumDegreesPerSecond, float deltaTime) noexcept;

	[[nodiscard]] sf::Vector2f GetForwardDirection() const noexcept;

	virtual void Update(float deltaTime) = 0;

	virtual bool IsCollideWith(const Entity& other) const = 0;
	[[nodiscard]] bool CheckCollision(const Entity& other) const noexcept;

	void Move(float deltaTime) noexcept;
	void Accelerate(const sf::Vector2f& delta) noexcept;
	void SetVisible(bool isVisible) noexcept;
	void FlashOnHit(float duration) noexcept;

	virtual void OnDestroy();

private:
	sf::Sprite sprite;
	sf::Vector2f velocity{ 0.f, 0.f };
	sf::Vector2f impulseVelocity{ 0.f, 0.f };

	Assets& assets;
	World& world;
	sf::Shader& hitFlashShader;

	bool isAlive = true;
	bool isVisible = true;

	float hitFlashRemaining = 0.f;
	float hitFlashDuration = 0.f;

	float collisionRadius = 0.f;
	std::vector<Collision::LocalCircle> collisionCircles;

	float visualScale = 1.f;

private:
	[[nodiscard]] sf::Vector2f GetCollisionCircleCenter(const Collision::LocalCircle& circle) const noexcept;
	[[nodiscard]] std::optional<Collision::CircleContactInfo> GetCollisionContactInfo(
		const Entity& other) const noexcept;

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	void UpdateEffects(float deltaTime) noexcept;

	friend class World;
};