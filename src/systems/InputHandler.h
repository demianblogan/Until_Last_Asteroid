#pragma once

#include <functional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "ActionMap.h"

template <typename Action>
class InputHandler
{
public:
	using Callback = std::function<void()>;

	explicit InputHandler(const ActionMap<Action>& map)
		: actionMap(map)
	{}

	void Subscribe(Action action, Callback callback)
	{
		callbacks[action].push_back(std::move(callback));
	}

	void UnsubscribeAll(Action action)
	{
		callbacks.erase(action);
	}

	void HandleEvent(const sf::Event& event)
	{
		const auto& bindings{ actionMap.GetBindingsMap() };

		for (const auto& [action, inputs] : bindings)
			for (const auto& input : inputs)
				if (MatchesEvent(input, event))
					Invoke(action);
	}

	void Update()
	{
		const auto& bindings{ actionMap.GetBindingsMap() };

		for (const auto& [action, inputs] : bindings)
			for (const auto& input : inputs)
				if (IsActive(input))
					Invoke(action);
	}

private:
	void Invoke(Action action)
	{
		auto iterator{ callbacks.find(action) };
		if (iterator != callbacks.end())
			for (auto& callback : iterator->second)
				callback();
	}

private:
	bool MatchesEvent(const InputAction& inputAction, const sf::Event& event) const
	{
		using TT = InputAction::TriggerType;

		if (inputAction.GetTrigger() == TT::WhileHeld)
			return false;

		const InputAction::TriggerType trigger = inputAction.GetTrigger();

		auto function{
			[trigger, &event](auto inputValue) -> bool
			{
				using T = decltype(inputValue);

				if constexpr (std::is_same_v<T, sf::Keyboard::Key>)
				{
					if (trigger == TT::OnPress)
					{
						if (const auto* e{ event.getIf<sf::Event::KeyPressed>() })
							return e->code == inputValue;
					}
					else if (trigger == TT::OnRelease)
					{
						if (const auto* e{ event.getIf<sf::Event::KeyReleased>() })
							return e->code == inputValue;
					}
				}
				else if constexpr (std::is_same_v<T, sf::Mouse::Button>)
				{
					if (trigger == TT::OnPress)
					{
						if (const auto* e{ event.getIf<sf::Event::MouseButtonPressed>() })
							return e->button == inputValue;
					}
					else if (trigger == TT::OnRelease)
					{
						if (const auto* e{ event.getIf<sf::Event::MouseButtonReleased>() })
							return e->button == inputValue;
					}
				}
				else
				{
					return false;
				}

				return false;
			}
		};

		return std::visit(function, inputAction.GetInput());
	}

	bool IsActive(const InputAction& inputAction) const
	{
		if (inputAction.GetTrigger() != InputAction::TriggerType::WhileHeld)
			return false;

		auto function{
			[](auto inputValue) -> bool
			{
				using T = decltype(inputValue);

				if constexpr (std::is_same_v<T, sf::Keyboard::Key>)
					return sf::Keyboard::isKeyPressed(inputValue);
				else if constexpr (std::is_same_v<T, sf::Mouse::Button>)
					return sf::Mouse::isButtonPressed(inputValue);
				else
					return false;
			}
		};

		return std::visit(function, inputAction.GetInput());
	}

private:
	const ActionMap<Action>& actionMap;
	std::unordered_map<Action, std::vector<Callback>> callbacks;
};
