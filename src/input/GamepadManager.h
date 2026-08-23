#pragma once

#include <optional>

#include <SFML/System/Vector2.hpp>

namespace sf { class Event; }

// Deliberately self-contained -- does not go through ActionMap/InputBinding/
// InputHandler like keyboard and mouse do. That trio is built specifically
// around rebindable, discrete keyboard/mouse input:
//   - InputBinding::InputValue can only hold a sf::Keyboard::Key or a
//     sf::Mouse::Button; there's no gamepad variant to add it to.
//   - Analog sticks aren't a discrete OnPress/OnRelease/WhileHeld trigger --
//     GetGameplayInput() below returns a continuous sf::Vector2f (with dead
//     zone handling), which ActionMap has nothing to match that against.
//   - Joystick button numbers aren't a stable named enum like
//     sf::Keyboard::Key; SFML only exposes raw indices, and what a given
//     index means depends on the controller's vendor (see the Xbox/
//     PlayStation-specific button numbers in GamepadManager.cpp), which
//     needs bespoke handling no simple action-to-code table could express.
//   - Menu navigation (NavigationAction below) has no equivalent in
//     Config::PlayerAction at all -- it's a separate concept only gamepads
//     (and no keyboard/mouse binding) need.
class GamepadManager
{
public:
    enum class Layout
    {
        Xbox,
        PlayStation,
        Generic
    };

    enum class NavigationAction
    {
        None,
        Up,
        Down,
        Left,
        Right,
        Confirm,
        Back
    };

    struct GameplayInput
    {
        sf::Vector2f movement;
        std::optional<sf::Vector2f> aimDirection;
        bool isFiring = false;
    };

    GamepadManager();

    void HandleEvent(const sf::Event& event);

    [[nodiscard]] GameplayInput GetGameplayInput() const;
    [[nodiscard]] NavigationAction GetNavigationAction(const sf::Event& event) const;

    [[nodiscard]] bool IsPausePressed(const sf::Event& event) const;
    [[nodiscard]] bool IsConnected() const noexcept;
    [[nodiscard]] bool IsInUse() const noexcept;
    [[nodiscard]] Layout GetLayout() const noexcept;

private:
    void RefreshConnection();
    [[nodiscard]] bool IsActiveJoystick(unsigned int joystickID) const noexcept;
    [[nodiscard]] bool IsButtonPressed(unsigned int button) const;
    [[nodiscard]] sf::Vector2f ReadStick(bool isRightStick) const;

    std::optional<unsigned int> activeJoystick;
    Layout layout = Layout::Generic;
    bool isInUse = false;
};