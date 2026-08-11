#pragma once

#include <string>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuButton.h"
#include "ui/NeonGlow.h"
#include "ui/ScreenFade.h"

class PlaceholderState : public State
{
public:
    ~PlaceholderState() override;
    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderOverlay() override;

protected:
    PlaceholderState(StateStack& stateStack, StateContext context, std::string titleText);

private:
    void GoBack();

    sf::RectangleShape background;
    sf::Text title;
    sf::Text message;
    MenuButton backButton;
    NeonGlow neonGlow;
    GlowingCursor menuCursor;
    ScreenFade screenFade;
    bool backRequested{ false };
};

class ScoresState final : public PlaceholderState
{
public:
    ScoresState(StateStack& stateStack, StateContext context);
};
