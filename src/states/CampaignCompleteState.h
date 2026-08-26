#pragma once

#include <vector>
#include <SFML/Graphics/Text.hpp>
#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/NineSliceFrame.h"
#include "ui/ScreenFade.h"

class CampaignCompleteState final : public State
{
public:
	CampaignCompleteState(StateStack& stateStack, StateContext context);
	~CampaignCompleteState() override;
	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;

private:
	void Activate();
	void ApplyReveal();
	UI::MenuBackground background;
	UI::NineSliceFrame titleFrame;
	UI::NineSliceFrame messageFrame;
	sf::Text title;
	std::vector<sf::Text> messageLines;
	std::vector<sf::Text> postscriptLines;
	UI::MenuButton button;
	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;
	UI::GlowingCursor cursor;
	UI::ScreenFade screenFade;
	float revealElapsed{ 0.f };
	float activationDelay{ 0.f };
	bool interactive{ false };
	bool leaving{ false };
};
