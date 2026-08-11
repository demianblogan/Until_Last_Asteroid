#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "ui/GlowingCursor.h"
#include "ui/MenuButton.h"
#include "ui/NeonGlow.h"

class AssetStore;
class AudioManager;
class GamepadManager;

namespace sf
{
    class Event;
    class RenderTarget;
    class RenderWindow;
}

class ResultScreen
{
public:
    enum class Mode
    {
        LevelComplete,
        Victory
    };

    enum class Action
    {
        Primary,
        MainMenu
    };

    ResultScreen(AssetStore& assets, AudioManager& audio, GamepadManager& gamepad,
        sf::Vector2f logicalSize);

    void Start(Mode mode, int level, int score);
    void Reset();
    void Update(float deltaTime);
    [[nodiscard]] std::optional<Action> HandleEvent(
        const sf::Event& event, sf::RenderWindow& window);
    void Draw(sf::RenderTarget& target);
    void DrawCursor(sf::RenderWindow& window);

    [[nodiscard]] bool IsActive() const noexcept;
    [[nodiscard]] Mode GetMode() const noexcept;

private:
    void SkipAnimation();
    void ApplyVisualState();
    void Select(std::size_t index, bool playSound = true);
    void SelectPrevious();
    void SelectNext();
    void UpdateMouseSelection(sf::Vector2f position);
    [[nodiscard]] std::optional<Action> ActivateSelected();
    void CenterText(sf::Text& text, sf::Vector2f position);

    AudioManager& audio;
    GamepadManager& gamepad;
    sf::Vector2f logicalSize;
    sf::RectangleShape shade;
    sf::Sprite titleFrame;
    sf::Text title;
    sf::Text summary;
    NeonGlow titleGlow;
    NeonGlow buttonGlow;
    GlowingCursor menuCursor;
    std::vector<MenuButton> buttons;
    std::size_t selectedIndex{ 0u };
    Mode mode{ Mode::LevelComplete };
    float elapsed{ 0.f };
    bool active{ false };
    bool interactive{ false };
};
