#include "Application.h"

#include <algorithm>
#include <array>
#include <vector>
#include <atomic>
#include <exception>
#include <filesystem>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#undef CreateWindow

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Sleep.hpp>
#include <SFML/Window/Context.hpp>
#include <SFML/Window/Event.hpp>

#include "states/CompanySplashState.h"
#include "states/AchievementsState.h"
#include "states/CreditsState.h"
#include "states/CampaignMenuState.h"
#include "states/CampaignCompleteState.h"
#include "states/GameplayState.h"
#include "states/LevelSelectState.h"
#include "states/LanguageSelectState.h"
#include "states/RecordsState.h"
#include "states/ShipUpgradesState.h"
#include "states/MainMenuState.h"
#include "states/OptionsState.h"
#include "states/PauseState.h"
#include "ui/TextWarmup.h"
#include "utils/ConfigEnums.h"

Application::RuntimeDirectoryInitializer::RuntimeDirectoryInitializer()
{
	// Windows' documented maximum length for a long (\\?\-prefixed) path,
	// including the terminating null -- large enough that GetModuleFileNameW
	// can never truncate the exe's own path, no matter how deeply nested.
	constexpr std::size_t MaxExecutablePathLength = 32768u;

	std::vector<wchar_t> executablePath(MaxExecutablePathLength);

	const DWORD executablePathLength = GetModuleFileNameW(
		nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size()));

	if (executablePathLength == 0u || executablePathLength >= executablePath.size())
		throw std::runtime_error("Unable to resolve executable path");

	// The exe normally sits a few folders below the game's root (e.g.
	// build/Debug/), so this walks upward from it, folder by folder,
	// looking for the one that actually contains assets/data -- that's the
	// directory every relative asset path in the game is written against,
	// so it's what gets set as the process's working directory once found.
	constexpr int MaxDirectoryWalkDepth = 6;

	std::filesystem::path directory(std::filesystem::path(executablePath.data()).parent_path());

	for (int depth = 0; depth < MaxDirectoryWalkDepth; depth++)
	{
		if (std::filesystem::is_directory(directory / "assets") &&
			std::filesystem::is_directory(directory / "assets" / "data"))
		{
			std::filesystem::current_path(directory);
			return;
		}

		const std::filesystem::path parent(directory.parent_path());
		if (parent == directory)
			break;
		directory = parent;
	}

	throw std::runtime_error("Unable to locate the game assets directory");
}

Application::Application()
	: localization(settings)
	, window(CreateWindow(settings.GetSettings().graphics))
	, audio(assets, settings)
	, display(window, LogicalSize)
	, stateStack(StateContext{
		window,
		assets,
		settings,
		campaignSave,
		records,
		achievements,
		localization,
		audio,
		display,
		LogicalSize,
		wasMainMenuIntroPlayed,
		gamepad,
		gamepadHaptics,
		gameplayLaunchRequest })
{
	gamepad.SetHaptics(&gamepadHaptics);

	display.RestoreLogicalView();

	// Loading state renders on explicit progress notifications. Frame limiting here
	// would turn every resource notification into an artificial frame wait.
	window.setVerticalSyncEnabled(false);
	window.setFramerateLimit(0u);

	// The first loading-screen frame is deferred until localization and fonts
	// are ready (see Assets::Initialize), so the progress bar and the
	// "Loading" text always appear together instead of the bar showing first
	// with text catching up a moment later.
	if (!localization.LoadCatalogs("assets/data/localization"))
		throw std::runtime_error("Unable to load localization catalogs");

	// Heavy asset decoding (textures, fonts, sounds, music, shaders) runs on a
	// background thread so the window keeps pumping events and the progress
	// bar keeps animating smoothly instead of freezing for the whole load.
	// sf::Context gives the thread its own OpenGL context; SFML shares GL
	// resources (textures, shaders) across contexts through its internal
	// shared context, so everything created here stays valid on the main
	// thread after the background thread joins.
	// The full loading bar is budgeted across every phase below, each
	// picking up exactly where the previous one left off so it fills
	// smoothly with no jump or backward snap: 0-70% asset loading (the
	// background thread below), 70-90% the UI::TextWarmup pass, 90-96% the
	// fixed "preparing_game" checkpoint (achievements/state setup has no
	// incremental progress of its own to report), 96-100% "ready".
	constexpr float AssetLoadingProgressShare = 0.7f;
	constexpr float TextWarmupProgressShare = 0.2f;

	std::atomic<float> loadingProgress = 0.f;
	std::atomic<bool> isLoadingFinished = false;

	// Points at a string literal (localization key) reported from the
	// background loader thread. A raw atomic pointer is safe here only
	// because every value ever stored is a literal with static storage
	// duration -- never a dynamically built std::string -- so there is no
	// lifetime issue reading it back from the main thread.
	std::atomic<const char*> loadingStageKey = "";
	bool isLoadingSucceeded = false;
	std::exception_ptr loadingError;

	// jthread instead of thread: closing the window mid-load needs to tell
	// this thread to stop early, and stop_token is the standard vocabulary
	// for that instead of a hand-rolled atomic<bool> cancellation flag.
	std::jthread loaderThread([&](std::stop_token stopToken)
		{
			try
			{
				sf::Context backgroundContext;
				isLoadingSucceeded = assets.Initialize(
					[&](float assetProgress, std::string_view stageKey)
					{
						loadingProgress.store(assetProgress * AssetLoadingProgressShare);
						loadingStageKey.store(stageKey.data());
						return !stopToken.stop_requested();
					});
			}
			catch (...)
			{
				loadingError = std::current_exception();
			}
			loadingProgress.store(AssetLoadingProgressShare);
			isLoadingFinished.store(true);
		});

	// How often the main thread wakes up to redraw the loading screen and
	// poll the background thread's progress while waiting for it.
	constexpr int LoadingPollIntervalMilliseconds = 8;

	while (!isLoadingFinished.load())
	{
		const float progress = loadingProgress.load();
		if (!RenderLoadingScreen(progress, loadingStageKey.load(), progress > 0.f))
		{
			loaderThread.request_stop();
			break;
		}
		sf::sleep(sf::milliseconds(LoadingPollIntervalMilliseconds));
	}
	loaderThread.join();

	if (loadingError)
		std::rethrow_exception(loadingError);

	if (!window.isOpen() || !isLoadingSucceeded)
	{
		return;
	}

	// Deliberately run here on the main thread rather than backgrounded
	// alongside asset loading like it used to be: this and the loading
	// screen's own per-frame text both touch the same sf::Font objects, and
	// SFML doesn't guarantee it's safe to render text on one thread while
	// another thread rasterizes new glyphs into that font's atlas texture
	// at the same time. Running concurrently was intermittently visible as
	// the loading text flashing "tofu" placeholder glyphs for a single
	// frame. Touches every (font, size) combination used anywhere in the
	// game, for every language, so the expensive first layout of each one
	// happens here -- still hidden behind the loading bar -- instead of
	// piecemeal the first time each screen (or a language switch,
	// including from the pause menu) needs it.
	isLoadingSucceeded = UI::TextWarmup::Run(assets, localization,
		[&](float warmupProgress, std::string_view stageKey)
		{
			return RenderLoadingScreen(
				AssetLoadingProgressShare + warmupProgress * TextWarmupProgressShare, stageKey, true);
		});
	if (!window.isOpen() || !isLoadingSucceeded)
	{
		return;
	}

	if (!achievements.LoadDefinitions("assets/data/achievement_definitions.json"))
		throw std::runtime_error("Unable to load achievement definitions");

	if (!achievements.LoadProgress())
		throw std::runtime_error("Unable to load achievement progress");

	achievementToast.emplace(assets, audio, achievements, localization, LogicalSize);

	audio.ApplySettings();

	campaignSave.UnlockNewLevel(assets.GetGameplayData().GetLevelCount());

	if (const CampaignProgress* progress = campaignSave.GetProgress())
		records.MergeCampaignScores(progress->levelBestScores);

	if (!RenderLoadingScreen(0.96f, "loading.preparing_game", true))
		return;

	FPSText.emplace(assets.Fonts().Get(Config::Font::MenuRegular), "FPS: --", 24);
	FPSText->setFillColor(sf::Color(130, 235, 245));
	FPSText->setOutlineColor(sf::Color(2, 10, 18, 220));
	FPSText->setOutlineThickness(2.f);

	const sf::FloatRect initialFPSBounds = FPSText->getLocalBounds();
	FPSText->setOrigin({ initialFPSBounds.position.x + initialFPSBounds.size.x, initialFPSBounds.position.y });
	FPSText->setPosition({ LogicalSize.x - 24.f, 18.f });

	stateStack.RegisterState<CompanySplashState>(StateID::CompanySplash);
	stateStack.RegisterState<LanguageSelectState>(StateID::LanguageSelect);
	stateStack.RegisterState<MainMenuState>(StateID::MainMenu);
	stateStack.RegisterState<AchievementsState>(StateID::Achievements);
	stateStack.RegisterState<CreditsState>(StateID::Credits);

	// Building these two states involves laying out several full-sentence
	// localized sf::Text objects; the first glyph-layout of previously
	// untouched text at their font/size is consistently expensive on this
	// SFML/driver combination (hundreds of ms, reproducibly, even on repeat
	// visits) and was showing up as a visible hitch every time they were
	// opened. Caching the built instance instead of rebuilding on every
	// visit avoids paying that cost more than once per state per run.
	stateStack.EnableStateCaching(StateID::Achievements);
	stateStack.EnableStateCaching(StateID::Credits);

	// NOTE: ShipUpgrades caching was tried here and reverted -- it broke the
	// "Continue" button on the second and later visits (root cause not yet
	// isolated). The real fix for its hitch was elsewhere (UI::ResultScreen's
	// text now gets its first, expensive layout at GameplayState
	// construction instead of at level-complete), so this isn't needed.
	stateStack.RegisterState<CampaignMenuState>(StateID::CampaignMenu);
	stateStack.RegisterState<LevelSelectState>(StateID::LevelSelect);
	stateStack.RegisterState<ShipUpgradesState>(StateID::ShipUpgrades);
	stateStack.RegisterState<RecordsState>(StateID::Records);
	stateStack.RegisterState<OptionsState>(StateID::Options, OptionsState::Origin::MainMenu);
	stateStack.RegisterState<OptionsState>(StateID::PauseOptions, OptionsState::Origin::PauseMenu);
	stateStack.RegisterState<GameplayState>(StateID::Gameplay);
	stateStack.RegisterState<CampaignCompleteState>(StateID::CampaignComplete);
	stateStack.RegisterState<PauseState>(StateID::Pause);

	stateStack.PushState(StateID::CompanySplash);
	stateStack.ApplyPendingChanges();

	// Deliberately ignoring the [[nodiscard]] result: nothing left to react to it with here.
	static_cast<void>(RenderLoadingScreen(1.f, "loading.ready", true));

	window.setVerticalSyncEnabled(settings.GetSettings().graphics.isVSyncEnabled);
	window.setFramerateLimit(
		settings.GetSettings().graphics.isVSyncEnabled ? 0u : settings.GetSettings().graphics.frameRateLimit);
}

sf::RenderWindow Application::CreateWindow(const GraphicsSettings& settings)
{
	sf::VideoMode mode(settings.resolution);

	if (settings.windowMode == WindowMode::Fullscreen && !mode.isValid())
		mode = sf::VideoMode::getDesktopMode();

	switch (settings.windowMode)
	{
	case WindowMode::Fullscreen:
		return sf::RenderWindow(mode, DisplayManager::WindowTitle, sf::Style::Default, sf::State::Fullscreen);

	case WindowMode::Windowed:
		return sf::RenderWindow(mode, DisplayManager::WindowTitle, sf::Style::Default, sf::State::Windowed);

	case WindowMode::Borderless:
		return sf::RenderWindow(
			sf::VideoMode::getDesktopMode(),
			DisplayManager::WindowTitle,
			sf::Style::None,
			sf::State::Windowed);

	default:
		return sf::RenderWindow(
			sf::VideoMode::getDesktopMode(),
			DisplayManager::WindowTitle,
			sf::Style::Default,
			sf::State::Fullscreen);
	}
}

void Application::ApplyWindowLifecycleEvent(const sf::Event& event)
{
	if (event.is<sf::Event::Resized>())
		display.RestoreLogicalView();
	else if (event.is<sf::Event::Closed>())
		window.close();
}

bool Application::RenderLoadingScreen(float progress, std::string_view stageKey, bool isFontAvailable)
{
	while (const std::optional<sf::Event> event = window.pollEvent())
		ApplyWindowLifecycleEvent(*event);
	if (!window.isOpen())
		return false;

	const float loadingTime = loadingAnimationClock.getElapsedTime().asSeconds();

	progress = std::clamp(progress, 0.f, 1.f);
	window.clear(sf::Color::Black);

	constexpr sf::Vector2f BarPosition{ 110.f, 972.f };
	constexpr sf::Vector2f BarSize{ 1700.f, 24.f };
	sf::RectangleShape barFrame(BarSize);

	barFrame.setPosition(BarPosition);
	barFrame.setFillColor({ 2, 10, 16, 255 });
	barFrame.setOutlineColor({ 48, 190, 225, 220 });
	barFrame.setOutlineThickness(2.f);

	window.draw(barFrame);

	constexpr float FillWidth = BarSize.x - 12.f;
	sf::RectangleShape barGlow({ FillWidth * progress, 12.f });
	barGlow.setPosition(BarPosition + sf::Vector2f{ 6.f, 6.f });
	barGlow.setFillColor({ 22, 185, 235, 80 });
	window.draw(barGlow);

	sf::RectangleShape barFill({ FillWidth * progress, 6.f });
	barFill.setPosition(BarPosition + sf::Vector2f{ 6.f, 9.f });
	barFill.setFillColor({ 78, 225, 250, 245 });
	window.draw(barFill);

	if (isFontAvailable)
	{
		const sf::Font& titleFont{ assets.Fonts().Get(localization.GetBoldFont(false)) };

		// Each of these loading-stage strings gets drawn at this font/size for
		// the first time somewhere in the loop below, one stage at a time as
		// the background thread reports progress. Touching them all here,
		// once, up front pays the first-touch glyph-rasterization cost (see
		// UI::TextWarmup.h) in a single hitch instead of as a brief garbled frame
		// every time a new stage label first appears.
		if (!areLoadingLabelsWarmedUp)
		{
			// Every localization key this loading screen ever displays as
			// its stage label -- keep this in sync with the stageKey values
			// reported by Assets::Initialize/UI::TextWarmup::Run below.
			static constexpr std::array LoadingStageKeys =
			{
				"loading.title",
				"loading.data",
				"loading.textures",
				"loading.sounds",
				"loading.music",
				"loading.shaders",
				"loading.interface",
				"loading.finalizing",
				"loading.preparing_text",
				"loading.preparing_game",
				"loading.ready"
			};

			for (const char* key : LoadingStageKeys)
			{
				sf::Text warmupText(titleFont, localization.GetText(key), 34u);

				// The bounds themselves are unused -- calling this forces sf::Text's lazy
				// glyph layout/rasterization to run now.
				static_cast<void>(warmupText.getLocalBounds());
			}
			areLoadingLabelsWarmedUp = true;
		}

		const auto centerText = [](sf::Text& text, sf::Vector2f position)
			{
				const sf::FloatRect bounds = text.getLocalBounds();
				text.setOrigin(bounds.position + bounds.size * 0.5f);
				text.setPosition(position);
			};

		// Slowly breathing opacity for the loading-stage label: oscillates
		// between fully transparent and fully opaque (sin, remapped from
		// -1..1 to 0..1) so the text fades in and out instead of sitting
		// static, then maps that into an alpha range that never goes fully
		// invisible (110-255) so the label stays legible throughout.
		const float loadingTextPulse = 0.5f + 0.5f * std::sin(loadingTime * 2.4f);
		const auto loadingTextAlpha = static_cast<std::uint8_t>(110.f + loadingTextPulse * 145.f);

		const sf::String& labelText = stageKey.empty()
			? localization.GetText("loading.title")
			: localization.GetText(stageKey);

		sf::Text loadingText(titleFont, labelText, 34u);
		loadingText.setFillColor({ 135, 225, 242, loadingTextAlpha });
		loadingText.setOutlineColor({ 0, 55, 72, loadingTextAlpha });
		loadingText.setOutlineThickness(2.f);

		centerText(loadingText, { LogicalSize.x * 0.5f, 925.f });

		window.draw(loadingText);
	}

	window.display();
	return true;
}

void Application::Run()
{
	sf::Clock clock;
	while (window.isOpen())
	{
		const float frameTime = clock.restart().asSeconds();
		const float deltaTime = std::min(frameTime, MaxFrameTime);

		HandleInput();
		if (!window.isOpen())
			break;

		Update(deltaTime, frameTime);
		if (!window.isOpen())
			break;

		Render();
	}
}

void Application::HandleInput()
{
	while (const std::optional<sf::Event> event = window.pollEvent())
	{
		gamepad.HandleEvent(*event);
		ApplyWindowLifecycleEvent(*event);

		if (!window.isOpen())
			return;

		stateStack.HandleEvent(*event);

		if (!window.isOpen())
			return;
	}
}

// frameTime (uncapped) is only for the FPS counter below; everything else uses the capped deltaTime.
void Application::Update(float deltaTime, float frameTime)
{
	stateStack.HandleRealtime();
	stateStack.Update(deltaTime);

	// Re-applied every frame (cheap, and correct as soon as the Controls ->
	// Gamepad settings toggles land) rather than once at startup, so a
	// setting change takes effect immediately without extra wiring.
	const GamepadSettings& gamepadSettings{ settings.GetSettings().gamepad };
	gamepadHaptics.SetVibrationEnabled(gamepadSettings.isVibrationEnabled);
	gamepadHaptics.SetLightbarEnabled(gamepadSettings.isControllerLightbarEnabled);
	gamepadHaptics.SetAdaptiveTriggersEnabled(gamepadSettings.isAdaptiveTriggersEnabled);

	// Gameplay drives the lightbar and the right trigger's adaptive
	// resistance itself (tracking player health / the laser being held, see
	// GameplayState::Update and Player::UpdateLaser); everywhere else --
	// menus, splash, loading -- the lightbar just shows a calm, constant
	// blue, and the trigger is force-released, so neither one is ever left
	// stuck on a stale value from a run that ended (e.g. the player dying
	// mid-laser) without Player itself getting a chance to release it.
	if (stateStack.GetTopStateID() != StateID::Gameplay)
	{
		if (gamepadHaptics.IsLightbarEnabled())
			gamepadHaptics.SetLightbarColor({ 40u, 110u, 255u });
		gamepadHaptics.SetRightTriggerSustainedResistance(false);
	}

	gamepadHaptics.Update(deltaTime);

	if (achievementToast)
		achievementToast->Update(deltaTime);

	if (stateStack.IsEmpty())
	{
		window.close();
		return;
	}

	audio.Update();
	UpdateFPSCounter(frameTime);
}

void Application::Render()
{
	window.clear();

	stateStack.Render();

	if (settings.GetSettings().graphics.needToShowFPS && FPSText.has_value())
		window.draw(*FPSText);

	stateStack.RenderOverlay();

	if (achievementToast)
		achievementToast->Draw(window);

	window.display();
}

void Application::UpdateFPSCounter(float frameTime)
{
	if (!FPSText.has_value())
		return;

	secondsSinceLastFPSUpdate += frameTime;
	framesSinceLastFPSUpdate++;

	if (secondsSinceLastFPSUpdate >= 1.f)
	{
		const unsigned int FPS = static_cast<unsigned int>(
			static_cast<float>(framesSinceLastFPSUpdate) / secondsSinceLastFPSUpdate + 0.5f);

		FPSText->setString("FPS: " + std::to_string(FPS));

		const sf::FloatRect bounds = FPSText->getLocalBounds();
		FPSText->setOrigin({ bounds.position.x + bounds.size.x, bounds.position.y });

		secondsSinceLastFPSUpdate = 0.f;
		framesSinceLastFPSUpdate = 0u;
	}
}