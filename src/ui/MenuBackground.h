#pragma once

#include <vector>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>

class Assets;

namespace sf
{
    class RenderTarget;
    class Shader;
    class Texture;
}

class MenuBackground
{
public:
    MenuBackground(Assets& assets, sf::Vector2f logicalSize);

    void SetMousePosition(sf::Vector2f position);
    void Update(float deltaTime);
    void Draw(sf::RenderTarget& target) const;

private:
    // One background star drifting behind the main menu.
    struct DecorativeStar
    {
        sf::CircleShape shape;
        float moveSpeed = 0.f;

        // How "close" this star appears to be, from 0 (far) to 1 (near).
        // Drives both its drift speed (set in InitializeStars) and how far it
        // shifts with the mouse-parallax offset (see Draw) -- closer stars
        // move faster and shift more, exactly like real-world parallax.
        float depth = 0.f;
    };

    // One decorative asteroid drifting behind the main menu (purely visual --
    // unrelated to gameplay asteroids, just reuses their textures).
    struct DecorativeAsteroid
    {
        DecorativeAsteroid(const sf::Texture& texture, float depth);

        sf::Sprite sprite;
        sf::Vector2f velocity;

        float rotationSpeed = 0.f;

        // Same meaning as DecorativeStar::depth above.
        float depth = 0.f;
    };

    void InitializeStars();
    void InitializeAsteroids(Assets& assets);

    void UpdateStars(float deltaTime);
    void UpdateAsteroids(float deltaTime);

    void DrawBackground(sf::RenderTarget& target) const;
    void DrawStars(sf::RenderTarget& target) const;
    void DrawAsteroids(sf::RenderTarget& target) const;
    void DrawVignette(sf::RenderTarget& target) const;

    sf::Sprite background;

    sf::RectangleShape vignette;
    sf::Shader& vignetteShader;

    sf::Vector2f logicalSize;

    sf::Vector2f targetParallax;
    sf::Vector2f currentParallax;

    std::vector<DecorativeStar> stars;
    std::vector<DecorativeAsteroid> asteroids;
};
