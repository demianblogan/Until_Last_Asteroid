#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics/Text.hpp>

#include "states/State.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "ui/MenuIntroAnimation.h"

namespace sf
{
    class Music;
}

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
    void Select(std::size_t index);
    void UpdateMouseSelection(sf::Vector2i pixelPosition);
    void ActivateSelected();
    void ApplyAnimationState();
    void HandleAnimationEvents(const MenuIntroAnimation::Events& events);
    void PlayTypingSounds(std::size_t count);
    void StartMenuMusic();

    MenuBackground background;
    MenuIntroAnimation introAnimation;
    sf::Text title;
    sf::Text version;
    std::vector<MenuButton> buttons;
    std::vector<sf::Sound> typingSounds;
    std::optional<sf::Sound> activationSound;
    sf::Music& menuMusic;
    std::size_t selectedIndex{ 0 };
    std::size_t typingSoundIndex{ 0 };
    float titleLeftPosition{ 0.f };
};
