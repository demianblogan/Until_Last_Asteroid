#include "Entity.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "assets/Assets.h"
#include "utils/ConfigEnums.h"

Entity::Entity(Assets& assets, World& world, sf::Texture& texture,
	float visualScale, float collisionRadius,
	std::span<const Collision::LocalCircle> configuredCollisionCircles)
	: sprite(texture)
	, assets(assets)
	, world(world)
	, hitFlashShader(assets.GetShader(Config::Shader::HitFlash))
	, collisionRadius(collisionRadius)
	, visualScale(visualScale)
	, collisionCircles(configuredCollisionCircles.begin(), configuredCollisionCircles.end())
{
	if (collisionCircles.empty())
		collisionCircles.push_back({ {}, collisionRadius });

	const sf::Vector2u size = texture.getSize();
	sf::Vector2f newOrigin(static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) * 0.5f);
	sprite.setOrigin(newOrigin);
	sprite.setScale({ visualScale, visualScale });
}

void Entity::SetPosition(const sf::Vector2f& position) noexcept
{
	sprite.setPosition(position);
}

sf::Vector2f Entity::GetPosition() const noexcept
{
	return sprite.getPosition();
}

void Entity::SetVelocity(const sf::Vector2f& newVelocity) noexcept
{
	velocity = newVelocity;
}

const sf::Vector2f& Entity::GetVelocity() const noexcept
{
	return velocity;
}

void Entity::ApplyImpulse(const sf::Vector2f& impulse) noexcept
{
	impulseVelocity += impulse;
}

void Entity::Translate(const sf::Vector2f& offset) noexcept
{
	sprite.move(offset);
}

bool Entity::IsAlive() const noexcept
{
	return isAlive;
}

void Entity::Destroy() noexcept
{
	if (isAlive)
	{
		isAlive = false;
		OnDestroy();
	}
}

const sf::Sprite& Entity::GetSprite() const noexcept
{
	return sprite;
}

World& Entity::GetWorld() noexcept
{
	return world;
}

Assets& Entity::GetAssets() noexcept
{
	return assets;
}

const World& Entity::GetWorld() const noexcept
{
	return world;
}

const Assets& Entity::GetAssets() const noexcept
{
	return assets;
}

void Entity::OnDestroy()
{
}

void Entity::Move(float deltaTime) noexcept
{
	sprite.move((velocity + impulseVelocity) * deltaTime);
}

float Entity::GetCollisionRadius() const noexcept
{
	return collisionRadius;
}

bool Entity::CheckCollision(const Entity& other) const noexcept
{
	return GetCollisionContactInfo(other).has_value();
}

sf::Vector2f Entity::GetCollisionCircleCenter(
	const Collision::LocalCircle& circle) const noexcept
{
	const float angle{ GetRotation().asRadians() };
	const float cosine{ std::cos(angle) };
	const float sine{ std::sin(angle) };
	return GetPosition() + sf::Vector2f{
		circle.offset.x * cosine - circle.offset.y * sine,
		circle.offset.x * sine + circle.offset.y * cosine };
}

std::optional<Collision::CircleContactInfo> Entity::GetCollisionContactInfo(
	const Entity& other) const noexcept
{
	std::optional<Collision::CircleContactInfo> deepestManifold;
	for (const Collision::LocalCircle& firstCircle : collisionCircles)
	{
		const sf::Vector2f firstCenter{ GetCollisionCircleCenter(firstCircle) };
		for (const Collision::LocalCircle& secondCircle : other.collisionCircles)
		{
			const auto manifold{ Collision::GetCircleContactInfo(
				firstCenter, firstCircle.radius,
				other.GetCollisionCircleCenter(secondCircle), secondCircle.radius) };
			if (manifold && (!deepestManifold ||
				manifold->penetration > deepestManifold->penetration))
			{
				deepestManifold = manifold;
			}
		}
	}
	return deepestManifold;
}

void Entity::Accelerate(const sf::Vector2f& delta) noexcept
{
	velocity += delta;
}

void Entity::SetVisible(bool visible) noexcept
{
	isVisible = visible;
}

void Entity::SetPresentation(
	float scaleMultiplier,
	float opacity,
	sf::Color tint) noexcept
{
	const float safeScale{ std::max(0.f, scaleMultiplier) };
	const auto alpha{ static_cast<std::uint8_t>(
		std::clamp(opacity, 0.f, 1.f) * 255.f) };
	sprite.setScale({ visualScale * safeScale, visualScale * safeScale });
	sprite.setColor(sf::Color(tint.r, tint.g, tint.b, alpha));
}

void Entity::FlashOnHit(float duration) noexcept
{
	hitFlashDuration = std::max(0.f, duration);
	hitFlashRemaining = hitFlashDuration;
}

void Entity::UpdateEffects(float deltaTime) noexcept
{
	hitFlashRemaining = std::max(0.f, hitFlashRemaining - deltaTime);

	static constexpr float ImpulseDampingPerSecond{ 7.f };
	impulseVelocity *= std::exp(-ImpulseDampingPerSecond * deltaTime);
}

void Entity::SetRotation(sf::Angle angle) noexcept
{
	sprite.setRotation(angle);
}

sf::Angle Entity::GetRotation() const noexcept
{
	return sprite.getRotation();
}

void Entity::TurnTowards(
	const sf::Vector2f& target,
	float maximumDegreesPerSecond,
	float deltaTime) noexcept
{
	const sf::Vector2f direction{ target - GetPosition() };
	if (direction.x * direction.x + direction.y * direction.y <= 0.0001f)
		return;

	const float desired{ std::atan2(direction.y, direction.x) +
		std::numbers::pi_v<float> * 0.5f };
	const float current{ GetRotation().asRadians() };
	const float difference{ std::atan2(
		std::sin(desired - current), std::cos(desired - current)) };
	const float maximumStep{ std::max(0.f, maximumDegreesPerSecond) *
		std::numbers::pi_v<float> / 180.f * std::max(0.f, deltaTime) };
	SetRotation(sf::radians(current + std::clamp(
		difference, -maximumStep, maximumStep)));
}

sf::Vector2f Entity::GetForwardDirection() const noexcept
{
	const float angle{ GetRotation().asRadians() -
		std::numbers::pi_v<float> * 0.5f };
	return { std::cos(angle), std::sin(angle) };
}

void Entity::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (!isVisible)
		return;

	if (hitFlashRemaining > 0.f && hitFlashDuration > 0.f)
	{
		hitFlashShader.setUniform("source", sf::Shader::CurrentTexture);
		hitFlashShader.setUniform("intensity", hitFlashRemaining / hitFlashDuration);
		states.shader = &hitFlashShader;
	}

	target.draw(sprite, states);
}
