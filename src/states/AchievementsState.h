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
	std::vector<UI::RoundedRectangleShape> tiles;
	// One glow per tile so each unlocked achievement can bloom independently --
	// only unlocked ones ever get drawn (see RenderTiles), the rest sit unused.
	std::vector<Rendering::NeonGlow> tileGlows;
	std::vector<sf::Sprite> icons;
	std::vector<sf::Text> achievementTitles;
	std::vector<sf::Text> descriptions;
	UI::MenuButton returnButton;
	std::size_t localizationRevision{ 0u };
	bool isReturnButtonSelected{ false };
};
