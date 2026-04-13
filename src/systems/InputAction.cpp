#include "InputAction.h"

InputAction::InputAction(sf::Keyboard::Key key, TriggerType trigger) noexcept
	: inputValue(key), trigger(trigger)
{}

InputAction::InputAction(sf::Mouse::Button button, TriggerType trigger) noexcept
	: inputValue(button), trigger(trigger)
{}

const InputAction::InputValue& InputAction::GetInput() const noexcept
{
	return inputValue;
}

InputAction::TriggerType InputAction::GetTrigger() const noexcept
{
	return trigger;
}