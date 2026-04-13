#pragma once

#include <SFML/Window/Keyboard.hpp>
#include "ActionMap.h"
#include "InputAction.h"
#include "utils/ConfigEnums.h"

// Creates default keyboard bindings for player actions
namespace InputBindings
{
	inline ActionMap<Config::PlayerAction> CreatePlayerBindings()
	{
		ActionMap<Config::PlayerAction> bindings;

		bindings.AddBinding(Config::PlayerAction::Up,
			InputAction(sf::Keyboard::Key::Up, InputAction::TriggerType::WhileHeld));

		bindings.AddBinding(Config::PlayerAction::Left,
			InputAction(sf::Keyboard::Key::Left, InputAction::TriggerType::WhileHeld));

		bindings.AddBinding(Config::PlayerAction::Right,
			InputAction(sf::Keyboard::Key::Right, InputAction::TriggerType::WhileHeld));

		bindings.AddBinding(Config::PlayerAction::Shoot,
			InputAction(sf::Keyboard::Key::Space, InputAction::TriggerType::OnPress));

		return bindings;
	}
}