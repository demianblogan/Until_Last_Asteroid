#include "State.h"

#include "StateStack.h"

State::State(StateStack& stateStack, StateContext context)
    : stateStack(stateStack)
    , context(context)
{
}

void State::HandleRealtime()
{
}

void State::RenderOverlay()
{
}

bool State::IsTransparent() const noexcept
{
    return false;
}

const StateContext& State::GetContext() const noexcept
{
    return context;
}

void State::RequestPush(StateId stateId)
{
    stateStack.PushState(stateId);
}

void State::RequestPop()
{
    stateStack.PopState();
}

void State::RequestClear()
{
    stateStack.ClearStates();
}
