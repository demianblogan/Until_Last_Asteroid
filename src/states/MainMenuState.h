#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "states/State.h"
#include "ui/MenuButton.h"

class MainMenuState final : public State
{
public:
    MainMenuState(StateStack& stateStack, StateContext context);

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    void SelectPrevious();
    void SelectNext();
    void Select(std::size_t index);
    void UpdateMouseSelection(sf::Vector2i pixelPosition);
    void ActivateSelected();

    sf::RectangleShape background;
    sf::Text title;
    sf::Text version;
    std::vector<MenuButton> buttons;
    std::size_t selectedIndex{ 0 };
};
