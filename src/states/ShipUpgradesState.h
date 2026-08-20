#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "campaign/ShipUpgrades.h"
#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuButton.h"
#include "ui/NeonGlow.h"
#include "ui/ScreenFade.h"

class ShipUpgradesState final : public State
{
public:
	ShipUpgradesState(StateStack& stateStack, StateContext context);

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;

private:
	enum class ExitTarget { None, Gameplay, MainMenu, LevelSelect };

	void Select(std::size_t index, bool playSound = true);
	void MoveSelection(int delta);
	void ActivateSelected();
	void Purchase(ShipUpgradeType type);
	void Refresh();
	void LayoutPartsPanel();
	void BeginExit(ExitTarget target);
	void PlayPressSound();

	sf::Sprite background;
	sf::Sprite headerDivider;
	NeonGlow glow;
	GlowingCursor cursor;
	ScreenFade fade;
	sf::Text title;
	MenuButton partsPanel;
	sf::Text partsLabel;
	sf::Text partsValue;
	sf::Sprite partsIcon;
	std::vector<MenuButton> upgradeRows;
	std::vector<sf::Sprite> upgradeIcons;
	std::vector<sf::Text> cardTitles;
	std::vector<sf::Text> cardDetails;
	std::vector<sf::Text> rankLabels;
	std::vector<sf::Text> rankValues;
	std::vector<sf::Text> rankMaximums;
	std::vector<sf::Text> cardCosts;
	std::vector<sf::RectangleShape> costDividers;
	std::vector<sf::Sprite> costIcons;
	std::vector<MenuButton> upgradeButtons;
	std::vector<sf::Text> maximumLabels;
	std::vector<MenuButton> buttons;
	std::array<bool, 4> maximumRanks{};
	std::size_t selectedIndex{ 0u };
	ExitTarget exitTarget{ ExitTarget::None };
	bool returnToLevelSelect{ false };
};
