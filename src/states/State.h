#pragma once

#include <SFML/System/Vector2.hpp>

#include "StateId.h"

class AssetStore;
class AudioManager;
class CampaignSaveManager;
class DisplayManager;
class SettingsManager;
class GamepadManager;
class RecordsManager;
class StateStack;
struct GameplayLaunchRequest;

namespace sf
{
    class Event;
    class RenderWindow;
}

struct StateContext
{
    sf::RenderWindow& window;
    AssetStore& assets;
    SettingsManager& settings;
    CampaignSaveManager& campaignSave;
	RecordsManager& records;
    AudioManager& audio;
    DisplayManager& display;
    sf::Vector2f logicalSize;
    bool& mainMenuIntroPlayed;
    GamepadManager& gamepad;
    GameplayLaunchRequest& gameplayLaunch;
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
    virtual void RenderOverlay();
    [[nodiscard]] virtual bool IsTransparent() const noexcept;

protected:
    [[nodiscard]] const StateContext& GetContext() const noexcept;

    void RequestPush(StateId stateId);
    void RequestPop();
    void RequestClear();

private:
    StateStack& stateStack;
    StateContext context;
};
