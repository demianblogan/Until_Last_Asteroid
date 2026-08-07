#pragma once

#include <SFML/System/Vector2.hpp>

#include "StateId.h"

class AssetStore;
class StateStack;

namespace sf
{
    class Event;
    class RenderWindow;
}

struct StateContext
{
    sf::RenderWindow& window;
    AssetStore& assets;
    sf::Vector2f logicalSize;
};

class State
{
public:
    State(StateStack& stateStack, StateContext context);
    virtual ~State() = default;

    State(const State&) = delete;
    State& operator=(const State&) = delete;
    State(State&&) = delete;
    State& operator=(State&&) = delete;

    virtual void HandleEvent(const sf::Event& event) = 0;
    virtual void HandleRealtime();
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;

protected:
    [[nodiscard]] const StateContext& GetContext() const noexcept;

    void RequestPush(StateId stateId);
    void RequestPop();
    void RequestClear();

private:
    StateStack& stateStack;
    StateContext context;
};
