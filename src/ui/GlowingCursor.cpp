#include "GlowingCursor.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"

namespace UI
{
	GlowingCursor::GlowingCursor(Assets& assets, Config::Texture texture, sf::Vector2f hotspot, sf::Color color)
		: sprite(assets.Textures().Get(texture)), glowEffect(assets), glowEffectColor(color)
	{
		sprite.setOrigin(hotspot);
	}

	void GlowingCursor::Update(float deltaTime)
	{
		glowEffect.Update(deltaTime);
	}

	void GlowingCursor::Draw(sf::RenderWindow& window)
	{
		if (!window.hasFocus())
			return;

		const sf::Vector2i pixelPosition = sf::Mouse::getPosition(window);
		const sf::Vector2u windowSize = window.getSize();

		if (pixelPosition.x < 0 || pixelPosition.y < 0 ||
			pixelPosition.x >= static_cast<int>(windowSize.x) ||
			pixelPosition.y >= static_cast<int>(windowSize.y))
		{
			return;
		}

		DrawAt(window, window.mapPixelToCoords(pixelPosition));
	}

	void GlowingCursor::DrawAt(sf::RenderWindow& window, sf::Vector2f position)
	{
		if (!window.hasFocus())
			return;

		sprite.setPosition(position);

		const sf::FloatRect bounds = sprite.getGlobalBounds();
		glowEffect.DrawBloom(window, bounds,
			[this](sf::RenderTarget& target, const sf::RenderStates& states)
			{
				target.draw(sprite, states);
			},
			glowEffectColor);

		window.draw(sprite);
		glowEffect.DrawHighlight(window, bounds, glowEffectColor);
	}
}