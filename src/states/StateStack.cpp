#include "StateStack.h"

#include <stdexcept>
#include <utility>
#include <vector>

StateStack::StateStack(StateContext context)
	: context(context)
{
}

void StateStack::EnableStateCaching(StateID stateID)
{
	cacheableStates.insert(stateID);
}

void StateStack::HandleEvent(const sf::Event& event)
{
	if (!states.empty())
		states.back().state->HandleEvent(event);

	ApplyPendingChanges();
}

void StateStack::HandleRealtime()
{
	if (!states.empty())
		states.back().state->HandleRealtime();

	ApplyPendingChanges();
}

void StateStack::Update(float deltaTime)
{
	if (!states.empty())
		states.back().state->Update(deltaTime);

	ApplyPendingChanges();
}

void StateStack::Render()
{
	if (states.empty())
		return;

	std::size_t firstVisibleState = states.size() - 1u;
	while (firstVisibleState > 0u && states[firstVisibleState].state->IsTransparent())
		firstVisibleState--;

	for (std::size_t index = firstVisibleState; index < states.size(); index++)
		states[index].state->Render();
}

void StateStack::RenderOverlay()
{
	if (!states.empty())
		states.back().state->RenderOverlay();
}

void StateStack::PushState(StateID stateID)
{
	pendingChanges.push_back({ Action::Push, stateID });
}

void StateStack::PopState()
{
	pendingChanges.push_back({ Action::Pop, std::nullopt });
}

void StateStack::ClearStates()
{
	pendingChanges.push_back({ Action::Clear, std::nullopt });
}

void StateStack::ApplyPendingChanges()
{
	std::vector<PendingChange> changes = std::move(pendingChanges);
	pendingChanges.clear();

	for (const PendingChange& change : changes)
	{
		switch (change.action)
		{
		case Action::Push:
		{
			const StateID stateID = change.stateID.value();
			if (const auto cached = cachedStates.find(stateID); cached != cachedStates.end())
			{
				std::unique_ptr<State> reactivated = std::move(cached->second);
				cachedStates.erase(cached);
				reactivated->OnReactivated();
				states.push_back({ stateID, std::move(reactivated) });
			}
			else
			{
				states.push_back({ stateID, CreateState(stateID) });
			}
			break;
		}

		case Action::Pop:
			if (!states.empty())
			{
				if (cacheableStates.contains(states.back().id))
					cachedStates[states.back().id] = std::move(states.back().state);
				states.pop_back();
			}
			break;

		case Action::Clear:
			for (StackEntry& entry : states)
			{
				if (cacheableStates.contains(entry.id))
					cachedStates[entry.id] = std::move(entry.state);
			}
			states.clear();
			break;
		}
	}
}

bool StateStack::IsEmpty() const noexcept
{
	return states.empty();
}

std::optional<StateID> StateStack::GetTopStateID() const noexcept
{
	return states.empty() ? std::nullopt : std::optional<StateID>{ states.back().id };
}

std::unique_ptr<State> StateStack::CreateState(StateID stateID)
{
	const auto factory = factories.find(stateID);
	if (factory == factories.end())
		throw std::logic_error("Attempted to create an unregistered application state");

	return factory->second();
}
