#pragma once

#include <vector>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

class AssetStore;

namespace sf
{
    class RenderTarget;
    class Texture;
}

class MenuBackground
{
public:
    MenuBackground(AssetStore& assets, sf::Vector2f logicalSize);

    void SetMousePosition(sf::Vector2f position);
    void Update(float deltaTime);
    void Draw(sf::RenderTarget& target) const;

private:
    struct Star
    {
        sf::CircleShape shape;
        float speed{ 0.f };
        float depth{ 0.f };
    };

    struct DecorativeAsteroid
    {
        DecorativeAsteroid(const sf::Texture& texture, float depth);

        sf::Sprite sprite;
        sf::Vector2f velocity;
        float rotationSpeed{ 0.f };
        float depth{ 0.f };
    };

    void InitializeStars();
    void InitializeAsteroids(AssetStore& assets);

    sf::Sprite background;
    sf::Vector2f logicalSize;
    sf::Vector2f targetParallax;
    sf::Vector2f currentParallax;
    std::vector<Star> stars;
    std::vector<DecorativeAsteroid> asteroids;
};
