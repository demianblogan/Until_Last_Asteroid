#include "DisplayManager.h"

#include <algorithm>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/VideoMode.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace
{
    constexpr const char* WindowTitle{ "Until last asteroid" };

    sf::View CreateLetterboxView(
        sf::Vector2f logicalSize,
        unsigned int windowWidth,
        unsigned int windowHeight)
    {
        sf::View view(sf::FloatRect({ 0.f, 0.f }, logicalSize));
        if (windowWidth == 0u || windowHeight == 0u)
            return view;

        const float windowRatio{ static_cast<float>(windowWidth) / static_cast<float>(windowHeight) };
        const float viewRatio{ logicalSize.x / logicalSize.y };
        sf::FloatRect viewport({ 0.f, 0.f }, { 1.f, 1.f });

        if (windowRatio > viewRatio)
        {
            viewport.size.x = viewRatio / windowRatio;
            viewport.position.x = (1.f - viewport.size.x) * 0.5f;
        }
        else
        {
            viewport.size.y = windowRatio / viewRatio;
            viewport.position.y = (1.f - viewport.size.y) * 0.5f;
        }

        view.setViewport(viewport);
        return view;
    }
}

void ConfigureBorderlessWindow(sf::RenderWindow& window, sf::Vector2u desktopSize)
{
    window.setPosition({ 0, 0 });

#ifdef _WIN32
    const HWND handle{ static_cast<HWND>(window.getNativeHandle()) };
    LONG_PTR style{ GetWindowLongPtrW(handle, GWL_STYLE) };
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    style |= WS_POPUP | WS_VISIBLE;
    SetWindowLongPtrW(handle, GWL_STYLE, style);

    LONG_PTR extendedStyle{ GetWindowLongPtrW(handle, GWL_EXSTYLE) };
    extendedStyle &= ~WS_EX_TOPMOST;
    extendedStyle |= WS_EX_APPWINDOW;
    SetWindowLongPtrW(handle, GWL_EXSTYLE, extendedStyle);

    SetWindowPos(
        handle,
        HWND_NOTOPMOST,
        0,
        0,
        static_cast<int>(desktopSize.x),
        static_cast<int>(desktopSize.y),
        SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOOWNERZORDER);
#endif
}

DisplayManager::DisplayManager(sf::RenderWindow& window, sf::Vector2f logicalSize)
    : window(window)
    , logicalSize(logicalSize)
{
    for (const sf::VideoMode& mode : sf::VideoMode::getFullscreenModes())
    {
        if (mode.size.x < 800u || mode.size.y < 600u)
            continue;

        if (std::ranges::find(supportedResolutions, mode.size) == supportedResolutions.end())
            supportedResolutions.push_back(mode.size);
    }

    const sf::Vector2u desktop{ sf::VideoMode::getDesktopMode().size };
    if (std::ranges::find(supportedResolutions, desktop) == supportedResolutions.end())
        supportedResolutions.insert(supportedResolutions.begin(), desktop);
}

const std::vector<sf::Vector2u>& DisplayManager::GetSupportedResolutions() const noexcept
{
    return supportedResolutions;
}

void DisplayManager::ConfigureExistingWindow(const GraphicsSettings& settings)
{
    if (settings.windowMode == WindowMode::Borderless)
        ConfigureBorderlessWindow(window, sf::VideoMode::getDesktopMode().size);
}

void DisplayManager::ApplyLiveSettings(const GraphicsSettings& settings)
{
    window.setVerticalSyncEnabled(settings.verticalSync);
    window.setFramerateLimit(settings.verticalSync ? 0u : settings.frameRateLimit);
}

void DisplayManager::ApplyDisplaySettings(const GraphicsSettings& settings)
{
    sf::VideoMode mode(settings.resolution);
    sf::State state{ sf::State::Windowed };
    std::uint32_t style{ sf::Style::Default };

    if (settings.windowMode == WindowMode::Fullscreen)
    {
        if (!mode.isValid())
            mode = sf::VideoMode::getDesktopMode();
        state = sf::State::Fullscreen;
    }
    else if (settings.windowMode == WindowMode::Borderless)
    {
        mode = sf::VideoMode::getDesktopMode();
        style = sf::Style::None;
    }

    window.create(mode, WindowTitle, style, state);
    ConfigureExistingWindow(settings);
    RestoreLogicalView();
    ApplyLiveSettings(settings);
}

void DisplayManager::RestoreLogicalView()
{
    window.setView(CreateLetterboxView(logicalSize, window.getSize().x, window.getSize().y));
}
