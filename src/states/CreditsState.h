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

class CreditsState final : public State
{
public:
	CreditsState(StateStack& stateStack, StateContext context);
	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;
	void OnReactivated() override;

private:
	void BeginReturn();
	void RefreshLocalizedContent();
	UI::MenuBackground background;
	UI::NineSliceFrame panel;
	NeonGlow titleGlow;
	NeonGlow buttonGlow;
	UI::GlowingCursor cursor;
	UI::ScreenFade fade;
	sf::Text title;
	std::vector<sf::Text> bodyLines;
	UI::MenuButton returnButton;
	std::size_t localizationRevision{ 0u };
	bool returnButtonSelected{ false };
	bool returning{ false };
};
