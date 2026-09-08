#pragma once

#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "states/MenuState.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/NineSliceFrame.h"

class CreditsState final : public MenuState
{
public:
	CreditsState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;
	void OnReactivated() override;

private:
	void BeginReturn();
	void RefreshLocalizedContent();

	UI::NineSliceFrame panel;
	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;

	sf::Text title;
	std::vector<sf::Text> bodyLines;
	UI::MenuButton returnButton;

	std::size_t localizationRevision = 0u;
	bool isReturnButtonSelected = false;
};
