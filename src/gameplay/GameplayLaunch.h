#pragma once

enum class GameplayLaunchMode
{
	ContinueCampaign,
	NewCampaign,
	Tutorial,
	SelectedLevel,
	Horde,
	Run
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
	bool isTutorialRunning{ false };
	bool needToReturnToLevelSelectAfterUpgrades{ false };
	int selectedLevel{ 1 };
};
