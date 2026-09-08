#pragma once

#include <vector>

#include <SFML/System/Vector2.hpp>

#include "settings/GameSettings.h"

namespace sf
{
    class RenderWindow;
}

class DisplayManager
{
public:
    // The one text every window this game ever creates or recreates is
    // titled with -- shared so Application::CreateWindow (the initial
    // window, built before a DisplayManager exists to recreate it later)
    // and ApplyDisplaySettings below can't drift out of sync with each other.
    static constexpr const char* WindowTitle = "Until Last Asteroid";

    DisplayManager(sf::RenderWindow& window, sf::Vector2f logicalSize);

    [[nodiscard]] const std::vector<sf::Vector2u>& GetSupportedResolutions() const noexcept;
    void ApplyLiveSettings(const GraphicsSettings& settings);
    void ApplyDisplaySettings(const GraphicsSettings& settings);

    // Recomputes the letterboxed view for the window's current size and
    // applies it. Public so callers can re-run it after a resize, not just
    // after ApplyDisplaySettings recreates the window.
    void RestoreLogicalView();

private:
    sf::RenderWindow& window;
    sf::Vector2f logicalSize;
    std::vector<sf::Vector2u> supportedResolutions;
};