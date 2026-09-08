#pragma once

#include <vector>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "states/MenuState.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class AchievementsState final : public MenuState
{
public:
	AchievementsState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;
	void OnReactivated() override;

private:
	void BeginReturn();
	void RefreshLocalizedContent();
	void RefreshUnlockState();

	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;

	sf::Text title;
	UI::MenuButton returnButton;

	// One entry per achievement, all index-aligned: the tile background, a
	// glow (only drawn for unlocked ones), the icon, and the title/description
	// text.
	std::vector<UI::RoundedRectangleShape> tiles;
	std::vector<Rendering::NeonGlow> tileGlows;
	std::vector<sf::Sprite> icons;
	std::vector<sf::Text> achievementTitles;
	std::vector<sf::Text> descriptions;

	std::size_t localizationRevision = 0u;
	bool isReturnButtonSelected = false;
};
