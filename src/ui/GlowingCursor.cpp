#include "GlowingCursor.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"

GlowingCursor::GlowingCursor(
    AssetStore& assets,
    Config::Texture texture,
    sf::Vector2f hotspot,
    sf::Color color)
    : sprite(assets.Textures().Get(texture))
    , glow(assets)
    , glowColor(color)
{
    sprite.setOrigin(hotspot);
}

void GlowingCursor::Update(float deltaTime)
{
    glow.Update(deltaTime);
}

void GlowingCursor::Draw(sf::RenderWindow& window)
{
    if (!window.hasFocus())
        return;

    const sf::Vector2i pixelPosition{ sf::Mouse::getPosition(window) };
    const sf::Vector2u windowSize{ window.getSize() };
    if (pixelPosition.x < 0 || pixelPosition.y < 0 ||
        pixelPosition.x >= static_cast<int>(windowSize.x) ||
        pixelPosition.y >= static_cast<int>(windowSize.y))
    {
        return;
    }

    sprite.setPosition(window.mapPixelToCoords(pixelPosition));
    const sf::FloatRect bounds{ sprite.getGlobalBounds() };
    glow.DrawBloom(
        window,
        bounds,
        [this](sf::RenderTarget& target, const sf::RenderStates& states)
        {
            target.draw(sprite, states);
        },
        glowColor);

    window.draw(sprite);
    glow.DrawHighlight(window, bounds, glowColor);
}
