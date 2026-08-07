#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "State.h"

class StateStack
{
public:
    explicit StateStack(StateContext context);

    template <typename StateType>
    void RegisterState(StateId stateId)
    {
        factories[stateId] = [this]
        {
            return std::make_unique<StateType>(*this, context);
        };
    }

    void HandleEvent(const sf::Event& event);
    void HandleRealtime();
    void Update(float deltaTime);
    void Render();

    void PushState(StateId stateId);
    void PopState();
    void ClearStates();
    void ApplyPendingChanges();

    [[nodiscard]] bool IsEmpty() const noexcept;

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
        std::optional<StateId> stateId;
    };

    using StateFactory = std::function<std::unique_ptr<State>()>;

    [[nodiscard]] std::unique_ptr<State> CreateState(StateId stateId);

    std::vector<std::unique_ptr<State>> states;
    std::vector<PendingChange> pendingChanges;
    std::unordered_map<StateId, StateFactory> factories;
    StateContext context;
};
