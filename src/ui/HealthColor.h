#pragma once

#include <SFML/Graphics/Color.hpp>

namespace UI
{
	// The player's health-bar fill color for a given health ratio (0..1),
	// low to high -- used by the HUD's own health bar.
	[[nodiscard]] sf::Color GetHealthColor(float ratio);

	// A bolder, more saturated green -> orange -> red take on the same
	// ratio, for a DualSense controller's lightbar. The HUD gradient above
	// passes through a pale yellow midpoint that reads as washed-out white
	// on the lightbar's diffuser, so this uses its own punchier stops
	// instead of sharing GetHealthColor's palette.
	[[nodiscard]] sf::Color GetHealthLightbarColor(float ratio);
}