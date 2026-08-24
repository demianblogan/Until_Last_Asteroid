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

namespace sf
{
	class Event;
}

class Application
{
public:
	Application();

	void Run();

private:
	// Exists only to run its constructor before every other member below --
	// C++ guarantees members construct in declaration order, and this is
	// declared first. Its constructor changes the process's working
	// directory to the game's own folder (so relative paths like
	// "assets/data/..." resolve correctly regardless of how the exe was
	// launched), which must happen before settings/localization/etc.
	// construct and start reading files. A plain function call in the
	// constructor body would run too late: the body only starts after
	// every member's own initializer has already run.
	struct RuntimeDirectoryInitializer
	{
		RuntimeDirectoryInitializer();
	};

	[[nodiscard]] static sf::RenderWindow CreateWindow(const GraphicsSettings& settings);
	[[nodiscard]] bool RenderLoadingScreen(float progress, std::string_view stageKey, bool isFontAvailable);
	void UpdateFPSCounter(float frameTime);

	// The three phases of one Run() frame. Each may close the window
	// (a Closed event, or the state stack running empty) partway through,
	// so Run() re-checks window.isOpen() between them and skips the rest
	// of the frame rather than calling Update()/Render() on a closing window.
	void HandleInput();
	void Update(float deltaTime, float frameTime);
	void Render();

	// Reacts to the two event types that affect the window itself rather
	// than any particular game state: a resize needs the letterbox view
	// recomputed, and a close request closes the window. Shared by the
	// loading-screen event loop and Run()'s main event loop.
	void ApplyWindowLifecycleEvent(const sf::Event& event);

	static constexpr sf::Vector2f LogicalSize = { 1920.f, 1080.f };

	// Upper bound on the delta time fed into a single Update() call,
	// regardless of how long that frame actually took to arrive. Without
	// this, a stall (window drag, minimize, a debugger breakpoint, an OS
	// hiccup) would hand gameplay one huge delta time and make everything
	// jump forward in a single leap -- entities skipping clean through
	// collisions, timers expiring instantly. Capping it just makes that one
	// frame look like a brief slowdown instead.
	static constexpr float MaxFrameTime = 0.1f;

	// Bootstrap and persistent, file-backed data services.
	RuntimeDirectoryInitializer runtimeDirectoryInitializer;
	SettingsManager settings;
	LocalizationManager localization;
	CampaignSaveManager campaignSave;
	RecordsManager records;
	AchievementManager achievements;

	// Window/rendering/audio infrastructure.
	sf::RenderWindow window;
	Assets assets;
	AudioManager audio;
	DisplayManager display;

	// Runtime session state and subsystems.
	bool wasMainMenuIntroPlayed = false;
	GamepadManager gamepad;
	GameplayLaunchRequest gameplayLaunchRequest;
	StateStack stateStack;
	std::optional<AchievementToast> achievementToast;

	// HUD/loading-screen presentation and its own bookkeeping.
	std::optional<sf::Text> FPSText;
	sf::Clock loadingAnimationClock;
	bool areLoadingLabelsWarmedUp = false;
	float secondsSinceLastFPSUpdate = 0.f;
	unsigned int framesSinceLastFPSUpdate = 0u;
};
