#pragma once

namespace sf
{
	class Sprite;
}

namespace Collision
{
	[[nodiscard]] bool Circle(const sf::Sprite& first, const sf::Sprite& second);
}