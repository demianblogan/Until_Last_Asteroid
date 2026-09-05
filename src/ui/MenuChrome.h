#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>

#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuTheme.h"
#include "ui/ScreenFade.h"

class Assets;

namespace sf
{
	class RenderTarget;
	class RenderWindow;
}

namespace UI
{
	// The three things every front-end screen carries: the drifting parallax
	// background, the glowing mouse cursor, and the full-screen entrance/exit
	// fade. Bundled so a screen (or MenuState) owns one MenuChrome instead of
	// three members plus the identical per-frame Update / RenderOverlay wiring.
	class MenuChrome
	{
	public:
		MenuChrome(Assets& assets, sf::Vector2f logicalSize,
		sf::Color cursorGlow = MenuTheme::InterfaceGlow);

		void SetMousePosition(sf::Vector2f position);
		void Update(float deltaTime);

		void StartFadeIn(float duration);
		void StartFadeOut(float duration);
		[[nodiscard]] bool IsFading() const noexcept;

		void DrawBackground(sf::RenderTarget& target) const;

		// Cursor (unless a gamepad is driving the menu) followed by the fade,
		// in that order -- what every screen's RenderOverlay() does.
		void DrawOverlay(sf::RenderWindow& window, bool isGamepadInUse);

		[[nodiscard]] MenuBackground& Background() noexcept { return background; }
		[[nodiscard]] GlowingCursor& Cursor() noexcept { return cursor; }
		[[nodiscard]] ScreenFade& Fade() noexcept { return fade; }

	private:
		MenuBackground background;
		GlowingCursor cursor;
		ScreenFade fade;
	};
}
