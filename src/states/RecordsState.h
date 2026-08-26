#pragma once

#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"
#include "ui/ScreenFade.h"

class RecordsState final : public State
{
public:
	RecordsState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;

private:
	void BeginReturn();

	UI::MenuBackground background;
	NeonGlow titleGlow;
	NeonGlow buttonGlow;
	UI::GlowingCursor cursor;
	UI::ScreenFade fade;
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
	bool returning{ false };
	bool returnButtonSelected{ false };
};
