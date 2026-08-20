#pragma once

#include <string_view>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "ui/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class AssetStore;

namespace sf { class RenderTarget; }

class LevelIntro
{
public:
	LevelIntro(AssetStore& assets, sf::Vector2f logicalSize);

	void Start(int levelNumber, std::string_view levelTitle);
	void StartMode(std::string_view modeName, std::string_view objective);
	[[nodiscard]] bool Update(float deltaTime);
	void Draw(sf::RenderTarget& target);
	void Reset() noexcept;

	[[nodiscard]] bool IsActive() const noexcept;

private:
	void StartWithText(std::string_view heading, std::string_view subtitle);
	void ApplyAnimation();
	static void CenterText(sf::Text& text, sf::Vector2f position);

	NeonGlow levelGlow;
	NeonGlow titleGlow;
	sf::RectangleShape shade;
	RoundedRectangleShape panel;
	sf::RectangleShape upperLine;
	sf::RectangleShape lowerLine;
	const sf::Font& levelTitleFont;
	const sf::Font& modeObjectiveFont;
	sf::Text levelLabel;
	sf::Text title;
	sf::Vector2f logicalSize;
	float elapsed{ 0.f };
	bool active{ false };
};
