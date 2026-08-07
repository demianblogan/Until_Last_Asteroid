#include "StateStack.h"

#include <stdexcept>
#include <utility>

StateStack::StateStack(StateContext context)
    : context(context)
{
}

void StateStack::HandleEvent(const sf::Event& event)
{
    if (!states.empty())
        states.back()->HandleEvent(event);

    ApplyPendingChanges();
}

void StateStack::HandleRealtime()
{
    if (!states.empty())
        states.back()->HandleRealtime();

    ApplyPendingChanges();
}

void StateStack::Update(float deltaTime)
{
    if (!states.empty())
        states.back()->Update(deltaTime);

    ApplyPendingChanges();
}

void StateStack::Render()
{
    if (states.empty())
        return;

    std::size_t firstVisibleState{ states.size() - 1u };
    while (firstVisibleState > 0u && states[firstVisibleState]->IsTransparent())
        --firstVisibleState;

    for (std::size_t index{ firstVisibleState }; index < states.size(); ++index)
        states[index]->Render();
}

void StateStack::RenderOverlay()
{
    if (!states.empty())
        states.back()->RenderOverlay();
}

void StateStack::PushState(StateId stateId)
{
    pendingChanges.push_back({ Action::Push, stateId });
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
    auto changes{ std::move(pendingChanges) };
    pendingChanges.clear();

    for (const PendingChange& change : changes)
    {
        switch (change.action)
        {
        case Action::Push:
            states.push_back(CreateState(change.stateId.value()));
            break;

        case Action::Pop:
            if (!states.empty())
                states.pop_back();
            break;

        case Action::Clear:
            states.clear();
            break;
        }
    }
}

bool StateStack::IsEmpty() const noexcept
{
    return states.empty();
}

std::unique_ptr<State> StateStack::CreateState(StateId stateId)
{
    const auto factory{ factories.find(stateId) };
    if (factory == factories.end())
        throw std::logic_error("Attempted to create an unregistered application state");

    return factory->second();
}
