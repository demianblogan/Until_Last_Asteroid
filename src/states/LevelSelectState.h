#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "states/MenuState.h"
#include "ui/MenuButtonList.h"
#include "rendering/NeonGlow.h"

class LevelSelectState final : public MenuState
{
public:
	LevelSelectState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;

private:
	void SelectPrevious();
	void SelectNext();
	void Select(std::size_t index, bool playSound = true);
	void UpdateMouseSelection(sf::Vector2f position);
	void SyncPartsSelection();
	void ActivateSelected();
	void BeginLevel(int level);

	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;
	Rendering::NeonGlow partsGlow;
	sf::Text title;
	UI::MenuButtonList buttonList;
	std::vector<UI::MenuButton> partsFrames;
	std::vector<sf::Sprite> partsIcons;
	std::vector<sf::Text> partsCounts;
	std::vector<int> buttonLevels;
};
