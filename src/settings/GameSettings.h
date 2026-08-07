#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

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

enum class InputDevice
{
    Keyboard,
    Mouse
};

struct ControlBinding
{
    InputDevice device{ InputDevice::Keyboard };
    int code{ static_cast<int>(sf::Keyboard::Key::Unknown) };
};

struct ControlSettings
{
    ControlBinding moveUp{ InputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::W) };
    ControlBinding moveDown{ InputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::S) };
    ControlBinding moveLeft{ InputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::A) };
    ControlBinding moveRight{ InputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::D) };
    ControlBinding fire{ InputDevice::Mouse, static_cast<int>(sf::Mouse::Button::Left) };
};

struct GameSettings
{
    static constexpr int FORMAT_VERSION{ 1 };

    GraphicsSettings graphics;
    AudioSettings audio;
    ControlSettings controls;
};
