#pragma once

#include <unordered_map>
#include <utility>
#include <vector>
#include "InputAction.h"

template <typename Action>
class ActionMap
{
public:
	void AddBinding(Action action, InputAction binding);
	[[nodiscard]] const std::unordered_map<Action, std::vector<InputAction>>& GetBindingsMap() const noexcept;

private:
	std::unordered_map<Action, std::vector<InputAction>> actions;
};

template <typename Action>
void ActionMap<Action>::AddBinding(Action action, InputAction binding)
{
	actions[action].push_back(std::move(binding));
}

template <typename Action>
const std::unordered_map<Action, std::vector<InputAction>>& ActionMap<Action>::GetBindingsMap() const noexcept
{
	return actions;
}
