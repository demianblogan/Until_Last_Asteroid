#pragma once

#include <optional>
#include <string_view>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Vector2.hpp>

#include "assets/Assets.h"
#include "achievements/AchievementManager.h"
#include "audio/AudioManager.h"
#include "app/DisplayManager.h"
#include "campaign/CampaignSaveManager.h"
#include "gameplay/GameplayLaunch.h"
#include "records/RecordsManager.h"
#include "settings/SettingsManager.h"
#include "localization/LocalizationManager.h"
#include "states/StateStack.h"
#include "input/GamepadManager.h"
#include "ui/AchievementToast.h"

class Application
{
public:
    Application();

    void Run();

private:
	struct RuntimeDirectoryInitializer
	{
		RuntimeDirectoryInitializer();
	};

    [[nodiscard]] static sf::View GetLetterboxView(
        const sf::View& view,
        unsigned int windowWidth,
        unsigned int windowHeight);
    [[nodiscard]] static sf::RenderWindow CreateWindow(const GraphicsSettings& settings);
	[[nodiscard]] bool RenderLoadingScreen(
		float progress, std::string_view stageKey, bool fontAvailable);
    void UpdateFpsCounter(float deltaTime);

    static constexpr sf::Vector2f LOGICAL_SIZE{ 1920.f, 1080.f };
    static constexpr float MAX_FRAME_TIME{ 0.1f };

	RuntimeDirectoryInitializer runtimeDirectoryInitializer;
    SettingsManager settings;
    LocalizationManager localization;
    CampaignSaveManager campaignSave;
	RecordsManager records;
	AchievementManager achievements;
    sf::RenderWindow window;
    Assets assets;
    AudioManager audio;
    DisplayManager display;
    bool mainMenuIntroPlayed{ false };
    GamepadManager gamepad;
    GameplayLaunchRequest gameplayLaunch;
    StateStack stateStack;
	std::optional<AchievementToast> achievementToast;
    std::optional<sf::Text> fpsText;
	sf::Clock loadingAnimationClock;
	bool loadingLabelsWarmedUp{ false };
    float fpsElapsed{ 0.f };
    unsigned int fpsFrames{ 0u };
};
