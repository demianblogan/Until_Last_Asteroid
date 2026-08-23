#pragma once

#include <optional>
#include <SFML/System/Vector2.hpp>

namespace Collision
{
	struct LocalCircle
	{
		sf::Vector2f offset;
		float radius{ 1.f };
	};

	struct CircleManifold
	{
		sf::Vector2f normal;
		float penetration;
	};

	[[nodiscard]] bool Circle(
		const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius);
	[[nodiscard]] std::optional<CircleManifold> GetCircleManifold(
		const sf::Vector2f& firstCenter, float firstRadius,
		const sf::Vector2f& secondCenter, float secondRadius);
}
