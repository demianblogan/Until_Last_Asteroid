#include "Collision.h"

#include <algorithm>
#include <cmath>
#include <SFML/Graphics/Sprite.hpp>

namespace
{
	struct CircleGeometry
	{
		sf::Vector2f center;
		float radius;
	};

	CircleGeometry GetGeometry(const sf::Sprite& sprite)
	{
		const sf::Vector2f size(sprite.getTextureRect().size);
		const sf::Vector2f scale{ sprite.getScale() };
		return {
			sprite.getTransform().transformPoint({ size.x * 0.5f, size.y * 0.5f }),
			std::min(size.x * std::abs(scale.x), size.y * std::abs(scale.y)) * 0.5f
		};
	}
}

namespace Collision
{
	std::optional<CircleManifold> GetCircleManifold(
		const sf::Sprite& first, const sf::Sprite& second)
	{
		const CircleGeometry a{ GetGeometry(first) };
		const CircleGeometry b{ GetGeometry(second) };
		const sf::Vector2f delta{ b.center - a.center };
		const float distanceSquared{ delta.x * delta.x + delta.y * delta.y };
		const float radiusSum{ a.radius + b.radius };
		if (distanceSquared > radiusSum * radiusSum)
			return std::nullopt;

		if (distanceSquared <= 0.0001f)
			return CircleManifold{ { 1.f, 0.f }, radiusSum };

		const float distance{ std::sqrt(distanceSquared) };
		return CircleManifold{ delta / distance, radiusSum - distance };
	}

	bool Circle(const sf::Sprite& first, const sf::Sprite& second)
	{
		return GetCircleManifold(first, second).has_value();
	}
}
