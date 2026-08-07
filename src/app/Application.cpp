#include "Application.h"

#include <algorithm>
#include <optional>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include "states/CompanySplashState.h"
#include "states/GameplayState.h"
#include "states/MainMenuState.h"
#include "states/PauseState.h"
#include "states/PlaceholderStates.h"

Application::Application()
    : window(CreateWindow(settings.Get().graphics))
    , stateStack(StateContext{ window, assets, settings, LOGICAL_SIZE })
{
    const sf::View logicalView(sf::FloatRect({ 0.f, 0.f }, LOGICAL_SIZE));
    window.setView(GetLetterboxView(logicalView, window.getSize().x, window.getSize().y));
    window.setVerticalSyncEnabled(settings.Get().graphics.verticalSync);
    window.setFramerateLimit(
        settings.Get().graphics.verticalSync ? 0u : settings.Get().graphics.frameRateLimit);

    assets.Initialize();

    stateStack.RegisterState<CompanySplashState>(StateId::CompanySplash);
    stateStack.RegisterState<MainMenuState>(StateId::MainMenu);
    stateStack.RegisterState<ScoresState>(StateId::Scores);
    stateStack.RegisterState<OptionsState>(StateId::Options);
    stateStack.RegisterState<GameplayState>(StateId::Gameplay);
    stateStack.RegisterState<PauseState>(StateId::Pause);
    stateStack.PushState(StateId::CompanySplash);
    stateStack.ApplyPendingChanges();
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

void Application::Run()
{
    sf::Clock clock;

    while (window.isOpen())
    {
        const float deltaTime{ std::min(clock.restart().asSeconds(), MAX_FRAME_TIME) };

        while (const std::optional<sf::Event> event{ window.pollEvent() })
        {
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

        window.clear();
        stateStack.Render();
        window.display();
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
