#pragma once

#include <SFML/System/Vector2.hpp>

#include "StateID.h"

class Assets;
class AchievementManager;
class AudioManager;
class CampaignSaveManager;
class DisplayManager;
class SettingsManager;
class GamepadManager;
class LocalizationManager;
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
    Assets& assets;
    SettingsManager& settings;
    CampaignSaveManager& campaignSave;
	RecordsManager& records;
	AchievementManager& achievements;
    LocalizationManager& localization;
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

    // Called when a cached state (see StateStack::EnableStateCaching) is
    // pushed again instead of being freshly constructed. Override to reset
    // per-visit transient state (fade-in, selection, etc.) and refresh any
    // data that may have changed while the state was cached away.
    virtual void OnReactivated();

protected:
    [[nodiscard]] const StateContext& GetContext() const noexcept;

    void RequestPush(StateID stateID);
    void RequestPop();
    void RequestClear();

private:
    StateStack& stateStack;
    StateContext context;
};
