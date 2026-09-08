#pragma once

#include <cstddef>
#include <optional>

#include <SFML/Graphics/Text.hpp>

#include "states/MenuState.h"
#include "ui/MenuButtonList.h"
#include "ui/MenuIntroAnimation.h"
#include "rendering/NeonGlow.h"

class MainMenuState final : public MenuState
{
public:
	MainMenuState(StateStack& stateStack, StateContext context);
	~MainMenuState() override;

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;

private:
	void ActivateSelected();
	void CompleteActivation(std::size_t index);
	void ApplyAnimationState();
	void HandleAnimationEvents(const UI::MenuIntroAnimation::Events& events);
	void PlayTypingSounds(std::size_t count);
	void StartMenuMusic();
	void RefreshLocalizedLabels();

	Rendering::NeonGlow neonGlow;
	Rendering::NeonGlow titleNeonGlow;
	UI::MenuIntroAnimation introAnimation;

	sf::Text title;
	sf::Text version;
	UI::MenuButtonList buttonList;

	std::optional<std::size_t> pendingActivation;
	std::size_t typingSoundIndex = 0;
	float activationDelayRemaining = 0.f;
	float titleLeftPosition = 0.f;
	std::size_t localizationRevision = 0u;
	bool hasRefreshedLocalizedLabels = false;
};
