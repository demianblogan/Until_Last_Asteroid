#include "Application.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <filesystem>
#include <cmath>
#include <optional>
#include <stdexcept>
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
	std::array<wchar_t, 32768> executablePath{};
	const DWORD length{ GetModuleFileNameW(
		nullptr, executablePath.data(), static_cast<DWORD>(executablePath.size())) };
	if (length == 0u || length >= executablePath.size())
		throw std::runtime_error("Unable to resolve executable path");

	std::filesystem::path candidate{
		std::filesystem::path(executablePath.data()).parent_path() };
	for (int depth{ 0 }; depth < 6; ++depth)
	{
		if (std::filesystem::is_directory(candidate / "assets") &&
			std::filesystem::is_directory(candidate / "assets" / "data"))
		{
			std::filesystem::current_path(candidate);
			return;
		}

		const std::filesystem::path parent{ candidate.parent_path() };
		if (parent == candidate)
			break;
		candidate = parent;
	}

	throw std::runtime_error("Unable to locate the game assets directory");
}

Application::Application()
    : localization(settings)
    , window(CreateWindow(settings.GetSettings().graphics))
    , audio(assets, settings)
    , display(window, LOGICAL_SIZE)
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
        LOGICAL_SIZE,
        mainMenuIntroPlayed,
        gamepad,
        gameplayLaunch })
{
    display.ConfigureExistingWindow(settings.GetSettings().graphics);
    const sf::View logicalView(sf::FloatRect({ 0.f, 0.f }, LOGICAL_SIZE));
    window.setView(GetLetterboxView(logicalView, window.getSize().x, window.getSize().y));
	// Loading renders on explicit progress notifications. Frame limiting here
	// would turn every resource notification into an artificial frame wait.
	window.setVerticalSyncEnabled(false);
	window.setFramerateLimit(0u);

	// The first loading-screen frame is deferred until localization and fonts
	// are ready (see Assets::Initialize), so the progress bar and the
	// "Loading" text always appear together instead of the bar showing first
	// with text catching up a moment later.
	if (!localization.Load("assets/data/localization"))
		throw std::runtime_error("Unable to load localization catalogs");

	// Heavy asset decoding (textures, fonts, sounds, music, shaders) runs on a
	// background thread so the window keeps pumping events and the progress
	// bar keeps animating smoothly instead of freezing for the whole load.
	// sf::Context gives the thread its own OpenGL context; SFML shares GL
	// resources (textures, shaders) across contexts through its internal
	// shared context, so everything created here stays valid on the main
	// thread after the background thread joins.
	std::atomic<float> loadingProgress{ 0.f };
	std::atomic<bool> loadingCancelled{ false };
	std::atomic<bool> loadingFinished{ false };
	// Points at a string literal (localization key) reported from the
	// background loader thread. A raw atomic pointer is safe here only
	// because every value ever stored is a literal with static storage
	// duration -- never a dynamically built std::string -- so there is no
	// lifetime issue reading it back from the main thread.
	std::atomic<const char*> loadingStageKey{ "" };
	bool loadingSucceeded{ false };
	std::exception_ptr loadingError;

	std::thread loaderThread([&]
		{
			try
			{
				sf::Context backgroundContext;
				loadingSucceeded = assets.Initialize(
					[&](float assetProgress, std::string_view stageKey)
					{
						// Reserves the top 10% of the bar for TextWarmup below,
						// which used to run silently after this reached 90%
						// with no progress of its own -- the bar would freeze
						// there for however long warmup took.
						loadingProgress.store(assetProgress * 0.9f);
						loadingStageKey.store(stageKey.data());
						return !loadingCancelled.load();
					});
				// Touches every (font, size) combination used anywhere in the
				// game, for every language, so the expensive first layout of
				// each one happens here -- hidden behind the loading bar --
				// instead of piecemeal the first time each screen (or a
				// language switch, including from the pause menu) needs it.
				if (loadingSucceeded && !loadingCancelled.load())
				{
					loadingSucceeded = TextWarmup::Run(assets, localization,
						[&](float warmupProgress, std::string_view stageKey)
						{
							loadingProgress.store(0.9f + warmupProgress * 0.1f);
							loadingStageKey.store(stageKey.data());
							return !loadingCancelled.load();
						});
				}
			}
			catch (...)
			{
				loadingError = std::current_exception();
			}
			loadingProgress.store(1.f);
			loadingFinished.store(true);
		});

	while (!loadingFinished.load())
	{
		const float progress{ loadingProgress.load() };
		if (!RenderLoadingScreen(0.05f + progress * 0.76f, loadingStageKey.load(), progress > 0.f))
		{
			loadingCancelled.store(true);
			break;
		}
		sf::sleep(sf::milliseconds(8));
	}
	loaderThread.join();

	if (loadingError)
		std::rethrow_exception(loadingError);
	if (!window.isOpen() || !loadingSucceeded)
	{
		return;
	}
	if (!RenderLoadingScreen(0.90f, loadingStageKey.load(), true))
		return;
	if (!achievements.LoadDefinitions("assets/data/achievement_definitions.json"))
		throw std::runtime_error("Unable to load achievement definitions");
	if (!achievements.LoadProgress())
		throw std::runtime_error("Unable to load achievement progress");
	achievementToast.emplace(assets, audio, achievements, localization, LOGICAL_SIZE);
    audio.ApplySettings();
	campaignSave.UnlockNewLevel(assets.GetGameplayData().GetLevelCount());
	if (const CampaignProgress* progress{ campaignSave.GetProgress() })
		static_cast<void>(records.MergeCampaignScores(progress->levelBestScores));
	if (!RenderLoadingScreen(0.96f, "loading.preparing_game", true))
		return;

    fpsText.emplace(assets.Fonts().Get(Config::Font::MenuRegular), "FPS: --", 24);
    fpsText->setFillColor(sf::Color(130, 235, 245));
    fpsText->setOutlineColor(sf::Color(2, 10, 18, 220));
    fpsText->setOutlineThickness(2.f);
    const sf::FloatRect initialFpsBounds{ fpsText->getLocalBounds() };
    fpsText->setOrigin({
        initialFpsBounds.position.x + initialFpsBounds.size.x,
        initialFpsBounds.position.y
    });
    fpsText->setPosition({ LOGICAL_SIZE.x - 24.f, 18.f });

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
	// isolated). The real fix for its hitch was elsewhere (ResultScreen's
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
        return sf::RenderWindow(
            mode,
            "Until last asteroid",
            sf::Style::Default,
            sf::State::Fullscreen);

    case WindowMode::Windowed:
        return sf::RenderWindow(
            mode,
            "Until last asteroid",
            sf::Style::Default,
            sf::State::Windowed);

    case WindowMode::Borderless:
        return sf::RenderWindow(
            sf::VideoMode::getDesktopMode(),
            "Until last asteroid",
            sf::Style::None,
            sf::State::Windowed);
    }

    return sf::RenderWindow(
        sf::VideoMode::getDesktopMode(),
        "Until last asteroid",
        sf::Style::Default,
        sf::State::Fullscreen);
}

bool Application::RenderLoadingScreen(
	float progress, std::string_view stageKey, bool fontAvailable)
{
	while (const std::optional<sf::Event> event{ window.pollEvent() })
	{
		if (event->is<sf::Event::Closed>())
		{
			window.close();
			return false;
		}
		if (const auto* resized{ event->getIf<sf::Event::Resized>() })
		{
			const sf::View logicalView(sf::FloatRect({ 0.f, 0.f }, LOGICAL_SIZE));
			window.setView(GetLetterboxView(
				logicalView, resized->size.x, resized->size.y));
		}
	}
	if (!window.isOpen())
		return false;

	const float loadingTime{ loadingAnimationClock.getElapsedTime().asSeconds() };

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

	constexpr float FillWidth{ BarSize.x - 12.f };
	sf::RectangleShape barGlow({ FillWidth * progress, 12.f });
	barGlow.setPosition(BarPosition + sf::Vector2f{ 6.f, 6.f });
	barGlow.setFillColor({ 22, 185, 235, 80 });
	window.draw(barGlow);
	sf::RectangleShape barFill({ FillWidth * progress, 6.f });
	barFill.setPosition(BarPosition + sf::Vector2f{ 6.f, 9.f });
	barFill.setFillColor({ 78, 225, 250, 245 });
	window.draw(barFill);

	if (fontAvailable)
	{
		const sf::Font& titleFont{ assets.Fonts().Get(localization.BoldFont(false)) };

		// Each of these loading-stage strings gets drawn at this font/size for
		// the first time somewhere in the loop below, one stage at a time as
		// the background thread reports progress. Touching them all here,
		// once, up front pays the first-touch glyph-rasterization cost (see
		// TextWarmup.h) in a single hitch instead of as a brief garbled frame
		// every time a new stage label first appears.
		if (!loadingLabelsWarmedUp)
		{
			for (const char* key : { "loading.title", "loading.data", "loading.textures",
				"loading.sounds", "loading.music", "loading.shaders", "loading.interface",
				"loading.finalizing", "loading.preparing_text", "loading.preparing_game",
				"loading.ready" })
			{
				sf::Text warmupText(titleFont, localization.Get(key), 34u);
				static_cast<void>(warmupText.getLocalBounds());
			}
			loadingLabelsWarmedUp = true;
		}

		const auto centerText{ [](sf::Text& text, sf::Vector2f position)
			{
				const sf::FloatRect bounds{ text.getLocalBounds() };
				text.setOrigin(bounds.position + bounds.size * 0.5f);
				text.setPosition(position);
			} };

		const float pulse{ 0.5f + 0.5f * std::sin(loadingTime * 2.4f) };
		const auto alpha{ static_cast<std::uint8_t>(110.f + pulse * 145.f) };
		const sf::String& labelText{ stageKey.empty()
			? localization.Get("loading.title")
			: localization.Get(stageKey) };
		sf::Text loading(titleFont, labelText, 34u);
		loading.setFillColor({ 135, 225, 242, alpha });
		loading.setOutlineColor({ 0, 55, 72, alpha });
		loading.setOutlineThickness(2.f);
		centerText(loading, { LOGICAL_SIZE.x * 0.5f, 925.f });
		window.draw(loading);
	}

	window.display();
	return true;
}

void Application::Run()
{
    sf::Clock clock;
    while (window.isOpen())
    {
        const float frameTime{ clock.restart().asSeconds() };
        const float deltaTime{ std::min(frameTime, MAX_FRAME_TIME) };

        while (const std::optional<sf::Event> event{ window.pollEvent() })
        {
            gamepad.HandleEvent(*event);
            if (const auto* resized{ event->getIf<sf::Event::Resized>() })
            {
                const sf::View logicalView(sf::FloatRect({ 0.f, 0.f }, LOGICAL_SIZE));
                window.setView(GetLetterboxView(logicalView, resized->size.x, resized->size.y));
            }

            if (event->is<sf::Event::Closed>())
            {
                window.close();
                break;
            }

            stateStack.HandleEvent(*event);

            if (!window.isOpen())
                break;
        }

        if (!window.isOpen())
            break;

        stateStack.HandleRealtime();
        stateStack.Update(deltaTime);
		if (achievementToast) achievementToast->Update(deltaTime);
		if (stateStack.IsEmpty())
		{
			window.close();
			break;
		}
        audio.Update();
        UpdateFpsCounter(frameTime);

        window.clear();
        stateStack.Render();
        if (settings.GetSettings().graphics.needToShowFPS && fpsText.has_value())
            window.draw(*fpsText);
        stateStack.RenderOverlay();
		if (achievementToast) achievementToast->Draw(window);
        window.display();
    }
}

void Application::UpdateFpsCounter(float deltaTime)
{
    if (!fpsText.has_value())
        return;

    fpsElapsed += deltaTime;
    ++fpsFrames;
    if (fpsElapsed >= 1.f)
    {
        const auto fps{ static_cast<unsigned int>(
            static_cast<float>(fpsFrames) / fpsElapsed + 0.5f) };
        fpsText->setString("FPS: " + std::to_string(fps));
        const sf::FloatRect bounds{ fpsText->getLocalBounds() };
        fpsText->setOrigin({ bounds.position.x + bounds.size.x, bounds.position.y });
        fpsElapsed = 0.f;
        fpsFrames = 0u;
    }
}

sf::View Application::GetLetterboxView(
    const sf::View& view,
    unsigned int windowWidth,
    unsigned int windowHeight)
{
    if (windowWidth == 0 || windowHeight == 0)
        return view;

    const float windowRatio{ static_cast<float>(windowWidth) / static_cast<float>(windowHeight) };
    const float viewRatio{ view.getSize().x / view.getSize().y };

    float sizeX{ 1.f };
    float sizeY{ 1.f };
    float positionX{ 0.f };
    float positionY{ 0.f };

    if (windowRatio > viewRatio)
    {
        sizeX = viewRatio / windowRatio;
        positionX = (1.f - sizeX) * 0.5f;
    }
    else
    {
        sizeY = windowRatio / viewRatio;
        positionY = (1.f - sizeY) * 0.5f;
    }

    sf::View letterboxView{ view };
    letterboxView.setViewport(sf::FloatRect({ positionX, positionY }, { sizeX, sizeY }));
    return letterboxView;
}
