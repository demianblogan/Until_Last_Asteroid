#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "State.h"

class StateStack
{
public:
    explicit StateStack(StateContext context);

    template <typename StateType, typename... Arguments>
    void RegisterState(StateId stateId, Arguments... arguments)
    {
        auto capturedArguments{ std::make_tuple(std::move(arguments)...) };
        factories[stateId] = [this, capturedArguments = std::move(capturedArguments)]
        {
            return std::apply(
                [this](const auto&... unpacked)
                {
                    return std::make_unique<StateType>(*this, context, unpacked...);
                },
                capturedArguments);
        };
    }

    void HandleEvent(const sf::Event& event);
    void HandleRealtime();
    void Update(float deltaTime);
    void Render();
    void RenderOverlay();

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
