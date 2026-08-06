#include "Entity.h"

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

Entity::Entity(AssetStore& assets, World& world, sf::Texture& texture) noexcept
	: sprite(texture), assets(assets), world(world)
{
	const sf::Vector2u size = texture.getSize();
	sf::Vector2f newOrigin(static_cast<float>(size.x) * 0.5f, static_cast<float>(size.y) * 0.5f);
	sprite.setOrigin(newOrigin);
}

void Entity::SetPosition(const sf::Vector2f& position) noexcept
{
	sprite.setPosition(position);
}

sf::Vector2f Entity::GetPosition() const noexcept
{
	return sprite.getPosition();
}

void Entity::SetVelocity(const sf::Vector2f& velocity) noexcept
{
	this->velocity = velocity;
}

const sf::Vector2f& Entity::GetVelocity() const noexcept
{
	return velocity;
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

AssetStore& Entity::GetAssets() noexcept
{
	return assets;
}

void Entity::OnDestroy()
{
}

void Entity::Move(float deltaTime) noexcept
{
	sprite.move(velocity * deltaTime);
}

bool Entity::CheckCollision(const Entity& other) const noexcept
{
	return Collision::Circle(GetSprite(), other.GetSprite());
}

void Entity::Accelerate(const sf::Vector2f& delta) noexcept
{
	velocity += delta;
}

void Entity::SetVisible(bool visible) noexcept
{
	isVisible = visible;
}

void Entity::SetRotation(sf::Angle angle) noexcept
{
	sprite.setRotation(angle);
}

sf::Angle Entity::GetRotation() const noexcept
{
	return sprite.getRotation();
}

void Entity::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (isVisible)
		target.draw(sprite, states);
}