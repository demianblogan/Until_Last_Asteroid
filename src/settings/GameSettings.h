#pragma once

#include <SFML/System/Vector2.hpp>

enum class WindowMode
{
    Fullscreen,
    Windowed,
    Borderless
};

struct GraphicsSettings
{
    sf::Vector2u resolution{ 1920u, 1080u };
    WindowMode windowMode{ WindowMode::Fullscreen };
    bool showFps{ false };
    bool verticalSync{ true };
    unsigned int frameRateLimit{ 0u };
};

struct AudioSettings
{
    float musicVolume{ 100.f };
    float soundVolume{ 100.f };
};

struct GameSettings
{
    static constexpr int FORMAT_VERSION{ 1 };

    GraphicsSettings graphics;
    AudioSettings audio;
};
