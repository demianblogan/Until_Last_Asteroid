#include "Collision.h"

#include <cmath>

namespace Collision
{
	std::optional<CircleManifold> GetCircleManifold(
		const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius)
	{
		const sf::Vector2f delta{ secondCenter - firstCenter };
		const float distanceSquared{ delta.x * delta.x + delta.y * delta.y };
		const float radiusSum{ firstRadius + secondRadius };
		if (distanceSquared > radiusSum * radiusSum)
			return std::nullopt;

		if (distanceSquared <= 0.0001f)
			return CircleManifold{ { 1.f, 0.f }, radiusSum };

		const float distance{ std::sqrt(distanceSquared) };
		return CircleManifold{ delta / distance, radiusSum - distance };
	}

	bool Circle(
		const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius)
	{
		return GetCircleManifold(
			firstCenter, firstRadius, secondCenter, secondRadius).has_value();
	}
}
