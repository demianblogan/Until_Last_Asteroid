#pragma once

#include <SFML/Graphics/Color.hpp>

// Shared accent colours for the menu / front-end states. Every menu screen was
// re-declaring the same two RGB triples under its own local name
// (SelectionGlowColor / Amber / SelectionGold ... and InterfaceGlowColor /
// Cyan ...), which had already drifted apart in a couple of files. One
// definition each, here.
namespace UI::MenuTheme
{
	// Warm amber -- the glow / highlight colour of the currently selected button.
	inline constexpr sf::Color SelectionGlow{ 255, 178, 42 };

	// Cyan -- titles, the menu cursor, panel outlines, non-selection accents.
	inline constexpr sf::Color InterfaceGlow{ 25, 220, 255 };
}
