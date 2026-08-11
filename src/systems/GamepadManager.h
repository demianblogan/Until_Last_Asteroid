#pragma once

#include <optional>

#include <SFML/System/Vector2.hpp>

namespace sf { class Event; }

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
        Back,
        Pause
    };

    struct GameplayInput
    {
        sf::Vector2f movement;
        std::optional<sf::Vector2f> aimDirection;
        bool fire{ false };
    };

    GamepadManager();

    void HandleEvent(const sf::Event& event);
    [[nodiscard]] GameplayInput GetGameplayInput() const;
    [[nodiscard]] NavigationAction GetNavigationAction(const sf::Event& event) const;
    [[nodiscard]] bool IsPausePressed(const sf::Event& event) const;
    [[nodiscard]] bool IsConnected() const noexcept;
    [[nodiscard]] bool IsUsingGamepad() const noexcept;
    [[nodiscard]] Layout GetLayout() const noexcept;

private:
    void RefreshConnection();
    [[nodiscard]] bool IsActiveJoystick(unsigned int joystickId) const noexcept;
    [[nodiscard]] bool IsButtonPressed(unsigned int button) const;
    [[nodiscard]] sf::Vector2f ReadStick(bool aiming) const;

    std::optional<unsigned int> activeJoystick;
    Layout layout{ Layout::Generic };
    bool usingGamepad{ false };
};
