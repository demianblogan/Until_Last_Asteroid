#pragma once

#include <variant>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

class InputAction
{
public:
    enum class TriggerType
    {
        OnPress,   // active only once on pressing the button
        OnRelease, // active only once on releasing the button
        WhileHeld  // is active while the button is being pressed
    };

    using InputValue = std::variant<sf::Keyboard::Key, sf::Mouse::Button>;

    InputAction(sf::Keyboard::Key key, TriggerType trigger) noexcept;
    InputAction(sf::Mouse::Button button, TriggerType trigger) noexcept;

    [[nodiscard]] const InputValue& GetInput() const noexcept;
    [[nodiscard]] TriggerType GetTrigger() const noexcept;

private:
    InputValue inputValue;
    TriggerType trigger;
};