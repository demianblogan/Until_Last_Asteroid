#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButtonList.h"
#include "rendering/NeonGlow.h"
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
	void SyncPartsSelection();
	void ActivateSelected();
	void BeginLevel(int level);

	UI::MenuBackground background;
	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;
	Rendering::NeonGlow partsGlow;
	UI::GlowingCursor menuCursor;
	UI::ScreenFade screenFade;
	sf::Text title;
	UI::MenuButtonList buttonList;
	std::vector<UI::MenuButton> partsFrames;
	std::vector<sf::Sprite> partsIcons;
	std::vector<sf::Text> partsCounts;
	std::vector<int> buttonLevels;
	bool isLaunchingLevel{ false };
};
