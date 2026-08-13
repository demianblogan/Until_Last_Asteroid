#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "ui/NeonGlow.h"
#include "ui/ScreenFade.h"

class LevelSelectState final : public State
{
public:
	LevelSelectState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;

private:
	void SelectPrevious();
	void SelectNext();
	void Select(std::size_t index, bool playSound = true);
	void UpdateMouseSelection(sf::Vector2f position);
	void ActivateSelected();
	void BeginLevel(int level);

	MenuBackground background;
	NeonGlow titleGlow;
	NeonGlow buttonGlow;
	NeonGlow partsGlow;
	GlowingCursor menuCursor;
	ScreenFade screenFade;
	sf::Text title;
	std::vector<MenuButton> buttons;
	std::vector<MenuButton> partsFrames;
	std::vector<sf::Sprite> partsIcons;
	std::vector<sf::Text> partsCounts;
	std::vector<int> buttonLevels;
	std::size_t selectedIndex{ 0u };
	bool launchingLevel{ false };
};
