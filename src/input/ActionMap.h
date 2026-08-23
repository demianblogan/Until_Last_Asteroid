#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

#include "InputBinding.h"

// Maps each value of a game-specific Action enum to the list of physical
// inputs (keys/mouse buttons, each with its own trigger condition) that
// should fire it. This is just the lookup table -- it doesn't read events or
// state itself; InputHandler<Action> does that, using an ActionMap to know
// what to check for.
//
// Example:
//   enum class PlayerAction { MoveUp, Fire };
//
//   ActionMap<PlayerAction> actions;
//   actions.AddBinding(PlayerAction::MoveUp,
//       InputBinding(sf::Keyboard::Key::W, InputBinding::TriggerType::WhileHeld));
//   actions.AddBinding(PlayerAction::Fire,
//       InputBinding(sf::Mouse::Button::Left, InputBinding::TriggerType::OnPress));
//
//   InputHandler<PlayerAction> input(actions);
//   input.Subscribe(PlayerAction::Fire, [] { /* shoot */ });
template <typename Action>
class ActionMap
{
public:
	void AddBinding(Action action, InputBinding binding)
	{
		actions[action].push_back(std::move(binding));
	}

	[[nodiscard]] const std::unordered_map<Action, std::vector<InputBinding>>& GetBindingsMap() const noexcept
	{
		return actions;
	}

private:
	std::unordered_map<Action, std::vector<InputBinding>> actions;
};