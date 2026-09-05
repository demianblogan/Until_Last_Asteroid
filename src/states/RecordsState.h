#pragma once

#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "states/MenuState.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class RecordsState final : public MenuState
{
public:
	RecordsState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;

private:
	void BeginReturn();

	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;
	sf::Text title;
	UI::RoundedRectangleShape campaignPanel;
	UI::RoundedRectangleShape hordePanel;
	UI::RoundedRectangleShape runPanel;
	sf::Text campaignTitle;
	sf::Text hordeTitle;
	sf::Text runTitle;
	std::vector<sf::Text> levelLabels;
	std::vector<sf::Text> levelScores;
	std::vector<sf::Text> hordeLabels;
	std::vector<sf::Text> hordeValues;
	sf::Text runLabel;
	sf::Text runValue;
	UI::MenuButton returnButton;
	bool isReturnButtonSelected{ false };
};
