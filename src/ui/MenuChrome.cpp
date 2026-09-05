#include "MenuChrome.h"

#include <SFML/Graphics/RenderWindow.hpp>

#include "utils/ConfigEnums.h"

namespace UI
{
	MenuChrome::MenuChrome(Assets& assets, sf::Vector2f logicalSize, sf::Color cursorGlow)
		: background(assets, logicalSize)
		, cursor(assets, Config::Texture::MenuPointer, { 6.f, 2.f }, cursorGlow)
		, fade(logicalSize)
	{
	}

	void MenuChrome::SetMousePosition(sf::Vector2f position)
	{
		background.SetMousePosition(position);
	}

	void MenuChrome::Update(float deltaTime)
	{
		background.Update(deltaTime);
		cursor.Update(deltaTime);
		fade.Update(deltaTime);
	}

	void MenuChrome::StartFadeIn(float duration)
	{
		fade.StartFadeIn(duration);
	}

	void MenuChrome::StartFadeOut(float duration)
	{
		fade.StartFadeOut(duration);
	}

	bool MenuChrome::IsFading() const noexcept
	{
		return fade.IsActive();
	}

	void MenuChrome::DrawBackground(sf::RenderTarget& target) const
	{
		background.Draw(target);
	}

	void MenuChrome::DrawOverlay(sf::RenderWindow& window, bool isGamepadInUse)
	{
		if (!isGamepadInUse)
			cursor.Draw(window);

		fade.Draw(window);
	}
}
