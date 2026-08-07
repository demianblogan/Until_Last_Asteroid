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
    DisplayManager(sf::RenderWindow& window, sf::Vector2f logicalSize);

    [[nodiscard]] const std::vector<sf::Vector2u>& GetSupportedResolutions() const noexcept;
    void ApplyLiveSettings(const GraphicsSettings& settings);
    void ApplyDisplaySettings(const GraphicsSettings& settings);

private:
    void RestoreLogicalView();

    sf::RenderWindow& window;
    sf::Vector2f logicalSize;
    std::vector<sf::Vector2u> supportedResolutions;
};
