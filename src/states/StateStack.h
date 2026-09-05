#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "State.h"

class StateStack
{
public:
	explicit StateStack(StateContext context);

	template <typename StateType, typename... Arguments>
	void RegisterState(StateID stateID, Arguments... arguments)
	{
		auto capturedArguments{ std::make_tuple(std::move(arguments)...) };
		factories[stateID] = [this, capturedArguments = std::move(capturedArguments)]
		{
			return std::apply(
				[this](const auto&... unpacked)
				{
					return std::make_unique<StateType>(*this, context, unpacked...);
				},
				capturedArguments);
		};
	}

	// Marks a state id as cacheable: instead of being destroyed on pop and
	// rebuilt from scratch on the next push, the instance is kept alive and
	// reused, with State::OnReactivated() called to refresh transient/dynamic
	// state. Use this only for states whose content doesn't need to change
	// while off-stack in a way OnReactivated() can't cheaply refresh.
	void EnableStateCaching(StateID stateID);

	void HandleEvent(const sf::Event& event);
	void HandleRealtime();
	void Update(float deltaTime);
	void Render();
	void RenderOverlay();

	void PushState(StateID stateID);
	void PopState();
	void ClearStates();
	void ApplyPendingChanges();

	[[nodiscard]] bool IsEmpty() const noexcept;
	[[nodiscard]] std::optional<StateID> GetTopStateID() const noexcept;

private:
	enum class Action
	{
		Push,
		Pop,
		Clear
	};

	struct PendingChange
	{
		Action action;
		std::optional<StateID> stateID;
	};

	struct StackEntry
	{
		StateID id;
		std::unique_ptr<State> state;
	};

	using StateFactory = std::function<std::unique_ptr<State>()>;

	[[nodiscard]] std::unique_ptr<State> CreateState(StateID stateID);

	std::vector<StackEntry> states;
	std::unordered_map<StateID, std::unique_ptr<State>> cachedStates;
	std::unordered_set<StateID> cacheableStates;
	std::vector<PendingChange> pendingChanges;
	std::unordered_map<StateID, StateFactory> factories;
	StateContext context;
};
