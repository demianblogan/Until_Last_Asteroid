#include "Collision.h"

#include <cmath>

namespace Collision
{
	std::optional<CircleContactInfo> GetCircleContactInfo(const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius)
	{
		const sf::Vector2f delta = secondCenter - firstCenter;
		const float distanceSquared = delta.x * delta.x + delta.y * delta.y;
		const float radiusSum = firstRadius + secondRadius;

		if (distanceSquared > radiusSum * radiusSum)
			return std::nullopt;

		// Guards the delta/distance normalization below against division by ~0 when
		// the two centers coincide or nearly do. Squared because we're comparing
		// against distanceSquared, not distance: this is (0.01f * 0.01f), so centers
		// within 0.01 units of each other are treated as coincident.
		constexpr float CoincidentCenterEpsilonSquared = 0.0001f;
		if (distanceSquared <= CoincidentCenterEpsilonSquared)
			return CircleContactInfo{ { 1.f, 0.f }, radiusSum };

		const float distance = std::sqrt(distanceSquared);
		return CircleContactInfo{ delta / distance, radiusSum - distance };
	}

	bool Circle(const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius)
	{
		return GetCircleContactInfo(firstCenter, firstRadius, secondCenter, secondRadius).has_value();
	}
}