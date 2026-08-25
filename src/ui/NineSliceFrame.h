#pragma once

#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
	struct RenderStates;
	class Texture;
}

class NineSliceFrame
{
public:
	NineSliceFrame(const sf::Texture& texture, sf::FloatRect destinationBounds,
		unsigned int textureBorderSize, sf::Vector2f targetBorderSize);

	void SetColor(sf::Color color);

	void Draw(sf::RenderTarget& target) const;
	void Draw(sf::RenderTarget& target, const sf::RenderStates& states) const;
	void DrawBorder(sf::RenderTarget& target, const sf::RenderStates& states) const;

	[[nodiscard]] sf::FloatRect GetBounds() const noexcept;

private:
	std::vector<sf::Sprite> slices;
	sf::FloatRect bounds;
};
