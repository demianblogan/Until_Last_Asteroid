#include "MenuBackground.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <random>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>

#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr std::size_t StarCount{ 90 };
    constexpr float WrapMargin{ 180.f };
}

MenuBackground::DecorativeAsteroid::DecorativeAsteroid(const sf::Texture& texture, float asteroidDepth)
    : sprite(texture)
    , depth(asteroidDepth)
{
}

MenuBackground::MenuBackground(AssetStore& assets, sf::Vector2f size)
    : background(assets.Textures().Get(Config::Texture::MainMenuBackground))
    , logicalSize(size)
{
    const sf::Vector2u textureSize{ background.getTexture().getSize() };
    const float scale{ std::max(
        logicalSize.x * 1.04f / static_cast<float>(textureSize.x),
        logicalSize.y * 1.04f / static_cast<float>(textureSize.y)) };

    background.setOrigin({
        static_cast<float>(textureSize.x) * 0.5f,
        static_cast<float>(textureSize.y) * 0.5f
    });
    background.setScale({ scale, scale });
    background.setPosition(logicalSize * 0.5f);

    InitializeStars();
    InitializeAsteroids(assets);
}

void MenuBackground::SetMousePosition(sf::Vector2f position)
{
    const sf::Vector2f halfSize{ logicalSize * 0.5f };
    const sf::Vector2f normalized{
        std::clamp((position.x - halfSize.x) / halfSize.x, -1.f, 1.f),
        std::clamp((position.y - halfSize.y) / halfSize.y, -1.f, 1.f)
    };

    targetParallax = { -normalized.x * 18.f, -normalized.y * 12.f };
}

void MenuBackground::Update(float deltaTime)
{
    const float smoothing{ 1.f - std::exp(-5.f * deltaTime) };
    currentParallax += (targetParallax - currentParallax) * smoothing;

    for (Star& star : stars)
    {
        star.shape.move({ -star.speed * deltaTime, star.speed * 0.12f * deltaTime });
        sf::Vector2f position{ star.shape.getPosition() };

        if (position.x < -WrapMargin)
            position.x = logicalSize.x + WrapMargin;
        if (position.y > logicalSize.y + WrapMargin)
            position.y = -WrapMargin;

        star.shape.setPosition(position);
    }

    for (DecorativeAsteroid& asteroid : asteroids)
    {
        asteroid.sprite.move(asteroid.velocity * deltaTime);
        asteroid.sprite.rotate(sf::degrees(asteroid.rotationSpeed * deltaTime));

        sf::Vector2f position{ asteroid.sprite.getPosition() };
        if (position.x < -WrapMargin)
            position.x = logicalSize.x + WrapMargin;
        else if (position.x > logicalSize.x + WrapMargin)
            position.x = -WrapMargin;

        if (position.y < -WrapMargin)
            position.y = logicalSize.y + WrapMargin;
        else if (position.y > logicalSize.y + WrapMargin)
            position.y = -WrapMargin;

        asteroid.sprite.setPosition(position);
    }
}

void MenuBackground::Draw(sf::RenderTarget& target) const
{
    sf::RenderStates states;
    states.transform.translate(currentParallax * 0.25f);
    target.draw(background, states);

    for (const Star& star : stars)
    {
        states.transform = sf::Transform::Identity;
        states.transform.translate(currentParallax * star.depth);
        target.draw(star.shape, states);
    }

    for (const DecorativeAsteroid& asteroid : asteroids)
    {
        states.transform = sf::Transform::Identity;
        states.transform.translate(currentParallax * asteroid.depth);
        target.draw(asteroid.sprite, states);
    }
}

void MenuBackground::InitializeStars()
{
    std::mt19937 generator{ 0xA57E201D };
    std::uniform_real_distribution<float> xDistribution{ 0.f, logicalSize.x };
    std::uniform_real_distribution<float> yDistribution{ 0.f, logicalSize.y };
    std::uniform_real_distribution<float> radiusDistribution{ 0.6f, 1.8f };
    std::uniform_real_distribution<float> depthDistribution{ 0.25f, 1.f };

    stars.reserve(StarCount);
    for (std::size_t index{ 0 }; index < StarCount; ++index)
    {
        const float depth{ depthDistribution(generator) };
        const float radius{ radiusDistribution(generator) * depth };
        Star star{ sf::CircleShape(radius), 5.f + depth * 14.f, depth };
        star.shape.setOrigin({ radius, radius });
        star.shape.setPosition({ xDistribution(generator), yDistribution(generator) });
        star.shape.setFillColor(sf::Color(
            static_cast<std::uint8_t>(155.f + 70.f * depth),
            static_cast<std::uint8_t>(190.f + 55.f * depth),
            255,
            static_cast<std::uint8_t>(55.f + 105.f * depth)));
        stars.push_back(std::move(star));
    }
}

void MenuBackground::InitializeAsteroids(AssetStore& assets)
{
    using Config::Texture;

    const std::array<Texture, 4> textures{
        Texture::BigMeteor1,
        Texture::BigMeteor3,
        Texture::MediumMeteor1,
        Texture::SmallMeteor2
    };
    const std::array<sf::Vector2f, 4> positions{
        sf::Vector2f{ 1520.f, 150.f },
        sf::Vector2f{ 1180.f, 890.f },
        sf::Vector2f{ 720.f, 240.f },
        sf::Vector2f{ 1760.f, 720.f }
    };
    const std::array<sf::Vector2f, 4> velocities{
        sf::Vector2f{ -8.f, 2.f },
        sf::Vector2f{ 5.f, -3.f },
        sf::Vector2f{ -4.f, 1.f },
        sf::Vector2f{ 7.f, 3.f }
    };
    const std::array<float, 4> scales{ 0.8f, 0.62f, 0.48f, 0.7f };
    const std::array<float, 4> depths{ 0.65f, 0.5f, 0.35f, 0.8f };

    asteroids.reserve(textures.size());
    for (std::size_t index{ 0 }; index < textures.size(); ++index)
    {
        const sf::Texture& texture{ assets.Textures().Get(textures[index]) };
        DecorativeAsteroid asteroid(texture, depths[index]);
        const sf::Vector2u textureSize{ texture.getSize() };
        asteroid.sprite.setOrigin({
            static_cast<float>(textureSize.x) * 0.5f,
            static_cast<float>(textureSize.y) * 0.5f
        });
        asteroid.sprite.setPosition(positions[index]);
        asteroid.sprite.setScale({ scales[index], scales[index] });
        asteroid.sprite.setColor(sf::Color(160, 190, 215, static_cast<std::uint8_t>(35.f + depths[index] * 45.f)));
        asteroid.velocity = velocities[index];
        asteroid.rotationSpeed = index % 2 == 0 ? 3.5f + static_cast<float>(index) : -2.5f - static_cast<float>(index);
        asteroids.push_back(std::move(asteroid));
    }
}
