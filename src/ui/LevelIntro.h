#pragma once

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "rendering/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class Assets;
class LocalizationManager;

namespace sf { class RenderTarget; }

class LevelIntro
{
public:
	LevelIntro(Assets& assets, LocalizationManager& localization, sf::Vector2f logicalSize);

	void Start(int levelNumber);
	void StartMode(const sf::String& modeName, const sf::String& objective);
	[[nodiscard]] bool Update(float deltaTime);
	void Draw(sf::RenderTarget& target);
	void Reset() noexcept;

	[[nodiscard]] bool IsActive() const noexcept;

private:
	void StartWithText(const sf::String& heading, const sf::String& subtitle);
	void ApplyAnimation();
	static void CenterText(sf::Text& text, sf::Vector2f position);

	NeonGlow levelGlow;
	Assets& assets;
	LocalizationManager& localization;
	NeonGlow titleGlow;
	sf::RectangleShape shade;
	RoundedRectangleShape panel;
	sf::RectangleShape upperLine;
	sf::RectangleShape lowerLine;
	sf::Text levelLabel;
	sf::Text title;
	sf::Vector2f logicalSize;
	float elapsed{ 0.f };
	bool active{ false };
	bool titleGlowEnabled{ true };
};
