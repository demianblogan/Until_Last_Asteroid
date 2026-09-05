#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "settings/GameSettings.h"
#include "states/MenuState.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"

class LanguageSelectState final : public MenuState
{
public:
	LanguageSelectState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;
	void RenderOverlay() override;

private:
	void Select(std::size_t index, bool playSound = true);
	void ConfirmSelection();

	static constexpr std::array Languages{
		Language::English, Language::Spanish, Language::Russian,
		Language::Ukrainian, Language::Arabic };

	Rendering::NeonGlow glow;
	Rendering::NeonGlow buttonGlow;
	sf::Text title;
	sf::Text hint;
	std::vector<UI::MenuButton> buttons;
	std::size_t selectedIndex{ 0u };
	sf::Vector2f cursorPosition;
};
