#pragma once

#include <optional>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class Sprite;
}

namespace Collision
{
	struct CircleManifold
	{
		sf::Vector2f normal;
		float penetration;
	};

	[[nodiscard]] bool Circle(const sf::Sprite& first, const sf::Sprite& second);
	[[nodiscard]] std::optional<CircleManifold> GetCircleManifold(
		const sf::Sprite& first, const sf::Sprite& second);
}
