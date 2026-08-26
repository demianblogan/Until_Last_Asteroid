#pragma once
#include <vector>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"
#include "ui/ScreenFade.h"

class AchievementsState final : public State
{
public:
	AchievementsState(StateStack& stateStack, StateContext context);
	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;
	void OnReactivated() override;
private:
	void BeginReturn();
	void RefreshLocalizedContent();
	void RefreshUnlockState();
	UI::MenuBackground background;
	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow buttonGlow;
	UI::GlowingCursor cursor;
	UI::ScreenFade fade;
	sf::Text title;
	std::vector<UI::RoundedRectangleShape> tiles;
	std::vector<sf::Sprite> icons;
	std::vector<sf::Text> achievementTitles;
	std::vector<sf::Text> descriptions;
	UI::MenuButton returnButton;
	std::size_t localizationRevision{ 0u };
	bool returnButtonSelected{ false };
	bool returning{ false };
};
