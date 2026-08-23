#pragma once

#include <algorithm>

#include <SFML/Graphics/Text.hpp>

namespace TextLayout
{
	inline void FitWidth(sf::Text& text, float maximumWidth, unsigned int minimumSize = 14u)
	{
		const unsigned int original{ text.getCharacterSize() };
		const float originalWidth{ text.getLocalBounds().size.x };
		const sf::Vector2f currentScale{ text.getScale() };
		const float displayedWidth{ originalWidth * currentScale.x };
		if (displayedWidth <= maximumWidth || displayedWidth <= 0.f)
			return;

		const float minimumScale{ static_cast<float>(minimumSize) /
			static_cast<float>(original) };
		const float scale{ std::clamp(maximumWidth / displayedWidth,
			minimumScale / currentScale.x, 1.f) };
		// Scaling preserves the requested glyph size and reuses SFML's existing
		// font atlas. Changing character size here created another atlas for every
		// localized line and made text-heavy states expensive to construct.
		text.setScale(currentScale * scale);
	}
}
