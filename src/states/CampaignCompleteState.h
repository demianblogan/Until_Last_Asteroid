#pragma once

#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "states/MenuState.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/NineSliceFrame.h"

class CampaignCompleteState final : public MenuState
{
public:
	CampaignCompleteState(StateStack& stateStack, StateContext context);
	~CampaignCompleteState() override;

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;

private:
	void Activate();
	void ApplyReveal();

	UI::NineSliceFrame titleFrame;
	UI::NineSliceFrame messageFrame;
	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;

	sf::Text title;
	std::vector<sf::Text> messageLines;
	std::vector<sf::Text> postscriptLines;
	UI::MenuButton button;

	float revealElapsed = 0.f;
	bool isInteractive = false;
};
