#pragma once

#include <optional>

#include <SFML/System/Vector2.hpp>

namespace Collision
{
	// A collision circle defined in an entity's local space (offset from its own
	// position/rotation, not world space). An entity can own several of these to
	// approximate a non-circular shape with multiple circles.
	struct LocalCircle
	{
		sf::Vector2f offset;
		float radius = 1.f;
	};

	// The result of a circle-circle collision test, with enough detail to resolve
	// the overlap (unlike a plain bool hit test).
	struct CircleContactInfo
	{
		sf::Vector2f normal; // Direction to push the two circles apart along, from the 1st circle toward the 2nd.
		float penetration;   // How deep the circles currently overlap along that direction.
	};

	[[nodiscard]] bool Circle(
		const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius);

	[[nodiscard]] std::optional<CircleContactInfo> GetCircleContactInfo(
		const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius);
}