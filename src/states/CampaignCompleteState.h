#pragma once

#include <vector>
#include <SFML/Graphics/Text.hpp>
#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "ui/NeonGlow.h"
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
	static void CenterText(sf::Text& text, sf::Vector2f position);
	MenuBackground background;
	NineSliceFrame titleFrame;
	NineSliceFrame messageFrame;
	sf::Text title;
	std::vector<sf::Text> messageLines;
	std::vector<sf::Text> postscriptLines;
	MenuButton button;
	NeonGlow titleGlow;
	NeonGlow buttonGlow;
	GlowingCursor cursor;
	ScreenFade screenFade;
	float revealElapsed{ 0.f };
	float activationDelay{ 0.f };
	bool interactive{ false };
	bool leaving{ false };
};
