#pragma once

enum class GameplayLaunchMode
{
    ContinueCampaign,
    NewCampaign,
    Tutorial
};

enum class GameplayRuntimeCommand
{
    None,
    RestartLevel,
    SkipTutorial
};

struct GameplayLaunchRequest
{
    GameplayLaunchMode mode{ GameplayLaunchMode::ContinueCampaign };
    GameplayRuntimeCommand pendingCommand{ GameplayRuntimeCommand::None };
    bool tutorialRunning{ false };
};
