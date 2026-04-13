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
	void Unbind(Action action);
	void Clear() noexcept;

	[[nodiscard]] const std::vector<InputAction>& GetBindings(Action action) const noexcept;
	[[nodiscard]] bool Contains(Action action) const noexcept;
	const std::unordered_map<Action, std::vector<InputAction>>& GetBindingsMap() const noexcept;

private:
	std::unordered_map<Action, std::vector<InputAction>> actions;
};

template <typename Action>
void ActionMap<Action>::AddBinding(Action action, InputAction binding)
{
	actions[action].push_back(std::move(binding));
}

template <typename Action>
void ActionMap<Action>::Unbind(Action action)
{
	actions.erase(action);
}

template <typename Action>
void ActionMap<Action>::Clear() noexcept
{
	actions.clear();
}

template <typename Action>
const std::vector<InputAction>& ActionMap<Action>::GetBindings(Action action) const noexcept
{
	// Returned when action is not found to avoid dangling reference / allocations
	static const std::vector<InputAction> empty;

	auto iterator{ actions.find(action) };
	if (iterator != actions.end())
		return iterator->second;

	return empty;
}

template <typename Action>
bool ActionMap<Action>::Contains(Action action) const noexcept
{
	return actions.contains(action);
}

template <typename Action>
const std::unordered_map<Action, std::vector<InputAction>>& ActionMap<Action>::GetBindingsMap() const noexcept
{
	return actions;
}