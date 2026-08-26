#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "settings/GameSettings.h"
#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/ScreenFade.h"

class LanguageSelectState final : public State
{
public:
    LanguageSelectState(StateStack& stateStack, StateContext context);

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    void Select(std::size_t index, bool sound = true);
    void ConfirmSelection();

    static constexpr std::array Languages{
        Language::English, Language::Spanish, Language::Russian,
        Language::Ukrainian, Language::Arabic };

    UI::MenuBackground background;
    Rendering::NeonGlow glow;
    Rendering::NeonGlow buttonGlow;
    UI::GlowingCursor cursor;
    UI::ScreenFade fade;
    sf::Text title;
    sf::Text hint;
    std::vector<UI::MenuButton> buttons;
    std::size_t selected{ 0u };
    sf::Vector2f cursorPosition;
};
