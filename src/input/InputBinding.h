#pragma once

#include <variant>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

class InputBinding
{
public:
    enum class TriggerType
    {
        OnPress,   // active only once on pressing the button
        OnRelease, // active only once on releasing the button
        WhileHeld  // is active while the button is being pressed
    };

    using InputValue = std::variant<sf::Keyboard::Key, sf::Mouse::Button>;

    InputBinding(sf::Keyboard::Key key, TriggerType triggerType) noexcept;
    InputBinding(sf::Mouse::Button button, TriggerType triggerType) noexcept;

    [[nodiscard]] const InputValue& GetInputValue() const noexcept;
    [[nodiscard]] TriggerType GetTriggerType() const noexcept;

private:
    InputValue inputValue;
    TriggerType triggerType;
};