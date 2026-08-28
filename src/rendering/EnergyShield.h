#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
}

namespace Rendering
{
	void DrawEnergyShield(sf::RenderTarget& target,	sf::Vector2f center, float radius, float pulse,
		sf::Color shellColor, sf::Color outlineColor, sf::Color glowColor, float hexOpacity,
		sf::RenderStates states = {});
}