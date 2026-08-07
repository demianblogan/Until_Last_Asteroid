#pragma once

#include <optional>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "app/DisplayManager.h"
#include "settings/SettingsManager.h"
#include "states/StateStack.h"

class Application
{
public:
    Application();

    void Run();

private:
    [[nodiscard]] static sf::View GetLetterboxView(
        const sf::View& view,
        unsigned int windowWidth,
        unsigned int windowHeight);
    [[nodiscard]] static sf::RenderWindow CreateWindow(const GraphicsSettings& settings);
    void UpdateFpsCounter(float deltaTime);

    static constexpr sf::Vector2f LOGICAL_SIZE{ 1920.f, 1080.f };
    static constexpr float MAX_FRAME_TIME{ 0.1f };

    SettingsManager settings;
    sf::RenderWindow window;
    AssetStore assets;
    AudioManager audio;
    DisplayManager display;
    StateStack stateStack;
    std::optional<sf::Text> fpsText;
    float fpsElapsed{ 0.f };
    unsigned int fpsFrames{ 0u };
};
