#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

#include "ui/NeonGlow.h"
#include "utils/ConfigEnums.h"

class AssetStore;

namespace sf
{
    class RenderWindow;
}

class GlowingCursor
{
public:
    GlowingCursor(
        AssetStore& assets,
        Config::Texture texture,
        sf::Vector2f hotspot,
        sf::Color glowColor);

    void Update(float deltaTime);
    void Draw(sf::RenderWindow& window);

private:
    sf::Sprite sprite;
    NeonGlow glow;
    sf::Color glowColor;
};
