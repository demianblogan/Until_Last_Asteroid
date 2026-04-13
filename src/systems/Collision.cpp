#include "Collision.h"

#include <algorithm>
#include <cmath>
#include <SFML/Graphics/Sprite.hpp>

namespace Collision
{
	bool Circle(const sf::Sprite& first, const sf::Sprite& second)
	{
		// Assumes sprites have valid textures and texture rectangles set
		sf::Vector2f firstSize(first.getTextureRect().size);
		sf::Vector2f secondSize(second.getTextureRect().size);

		auto [firstScaleX, firstScaleY] { first.getScale() };
		auto [secondScaleX, secondScaleY] { second.getScale() };
		firstScaleX = std::abs(firstScaleX);
		firstScaleY = std::abs(firstScaleY);
		secondScaleX = std::abs(secondScaleX);
		secondScaleY = std::abs(secondScaleY);

		// Use an inscribed circle (fits inside the sprite bounds).
		// This reduces false positives but may miss collisions at corners.
		float radius1{ std::min(firstSize.x * firstScaleX, firstSize.y * firstScaleY) / 2.f };
		float radius2{ std::min(secondSize.x * secondScaleX, secondSize.y * secondScaleY) / 2.f };

		// Transform local center (half size) into world space.
		// This accounts for origin, scale, rotation and position.
		sf::Vector2f center1{ first.getTransform().transformPoint({ firstSize.x / 2.f, firstSize.y / 2.f }) };
		sf::Vector2f center2{ second.getTransform().transformPoint({ secondSize.x / 2.f, secondSize.y / 2.f }) };

		float deltaX{ center1.x - center2.x };
		float deltaY{ center1.y - center2.y };

		// Compare squared distance to avoid expensive sqrt.
		float distanceSquared{ deltaX * deltaX + deltaY * deltaY };
		float radiusSum{ radius1 + radius2 };

		return distanceSquared <= radiusSum * radiusSum;
	}
}