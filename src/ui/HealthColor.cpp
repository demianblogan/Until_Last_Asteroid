#include "HealthColor.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace UI
{
	namespace
	{
		// The health bar's fill color gradient, low to high (not sf::Color::Red/
		// Yellow/Green -- those are pure primaries; these are custom, softer tones
		// matching the game's neon palette).
		constexpr sf::Color HealthLowColor{ 255, 55, 48 };
		constexpr sf::Color HealthMidColor{ 255, 215, 45 };
		constexpr sf::Color HealthFullColor{ 55, 235, 105 };

		// Plain, unmixed red/orange/green for the lightbar gradient -- zero
		// blue component throughout (no blue tint), and no white/yellow
		// midpoint to wash out on the lightbar's diffuser like the HUD
		// gradient above does.
		constexpr sf::Color LightbarLowColor{ 255, 0, 0 };
		constexpr sf::Color LightbarMidColor{ 255, 130, 0 };
		constexpr sf::Color LightbarFullColor{ 0, 200, 0 };

		sf::Color LerpColor(const sf::Color& from, const sf::Color& to, float amount)
		{
			amount = std::clamp(amount, 0.f, 1.f);

			return sf::Color(
				static_cast<std::uint8_t>(std::lerp(from.r, to.r, amount)),
				static_cast<std::uint8_t>(std::lerp(from.g, to.g, amount)),
				static_cast<std::uint8_t>(std::lerp(from.b, to.b, amount)),
				static_cast<std::uint8_t>(std::lerp(from.a, to.a, amount)));
		}
	}

	sf::Color GetHealthColor(float ratio)
	{
		ratio = std::clamp(ratio, 0.f, 1.f);

		return ratio >= 0.5f
			? LerpColor(HealthMidColor, HealthFullColor, (ratio - 0.5f) * 2.f)
			: LerpColor(HealthLowColor, HealthMidColor, ratio * 2.f);
	}

	sf::Color GetHealthLightbarColor(float ratio)
	{
		ratio = std::clamp(ratio, 0.f, 1.f);

		return ratio >= 0.5f
			? LerpColor(LightbarMidColor, LightbarFullColor, (ratio - 0.5f) * 2.f)
			: LerpColor(LightbarLowColor, LightbarMidColor, ratio * 2.f);
	}
}
