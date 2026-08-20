#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
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

class GameOverScreen
{
public:
    enum class Action
    {
        RestartLevel,
        MainMenu
    };

    GameOverScreen(AssetStore& assets, AudioManager& audio, GamepadManager& gamepad,
        sf::Vector2f logicalSize);

    void Start(int finalScore);
    void StartHorde(int finalScore, int wavesSurvived);
    void StartRun(int survivalSeconds, int recordSeconds);
    void Reset();
    void Update(float deltaTime);
    [[nodiscard]] std::optional<Action> HandleEvent(
        const sf::Event& event, sf::RenderWindow& window);
    void Draw(sf::RenderTarget& target);
    void DrawCursor(sf::RenderWindow& window);

    [[nodiscard]] bool IsActive() const noexcept;

private:
    void StartWithSummary(std::string summary, std::string_view restartLabel);
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
    sf::Text finalScore;
    NeonGlow titleGlow;
    NeonGlow buttonGlow;
    GlowingCursor menuCursor;
    std::vector<MenuButton> buttons;
    std::size_t selectedIndex{ 0u };
    float elapsed{ 0.f };
    bool active{ false };
    bool interactive{ false };
};
