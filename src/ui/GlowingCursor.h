#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

#include "rendering/NeonGlow.h"
#include "utils/ConfigEnums.h"

class Assets;

namespace sf
{
	class RenderWindow;
}

namespace UI
{
	class GlowingCursor
	{
	public:
		GlowingCursor(Assets& assets, Config::Texture texture, sf::Vector2f hotspot, sf::Color glowColor);

		void Update(float deltaTime);
		void Draw(sf::RenderWindow& window);
		void DrawAt(sf::RenderWindow& window, sf::Vector2f position);

	private:
		sf::Sprite sprite;
		NeonGlow glowEffect;
		sf::Color glowEffectColor;
	};
}