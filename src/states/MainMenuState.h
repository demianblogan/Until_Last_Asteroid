#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "states/State.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "ui/MenuIntroAnimation.h"
#include "ui/NeonGlow.h"

class MainMenuState final : public State
{
public:
    MainMenuState(StateStack& stateStack, StateContext context);
    ~MainMenuState() override;

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    void SelectPrevious();
    void SelectNext();
    void Select(std::size_t index, bool playSound = true);
    void UpdateMouseSelection(sf::Vector2i pixelPosition);
    void ActivateSelected();
    void CompleteActivation(std::size_t index);
    void ApplyAnimationState();
    void HandleAnimationEvents(const MenuIntroAnimation::Events& events);
    void PlayTypingSounds(std::size_t count);
    void StartMenuMusic();

    MenuBackground background;
    NeonGlow neonGlow;
    MenuIntroAnimation introAnimation;
    sf::Text titleGlow;
    sf::Text title;
    sf::Text version;
    std::vector<MenuButton> buttons;
    std::optional<std::size_t> pendingActivation;
    std::size_t selectedIndex{ 0 };
    std::size_t typingSoundIndex{ 0 };
    float activationDelayRemaining{ 0.f };
    float titleLeftPosition{ 0.f };
};
