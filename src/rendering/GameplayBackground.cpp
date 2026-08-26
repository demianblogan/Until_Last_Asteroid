#include "GameplayBackground.h"

#include <algorithm>
#include <random>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include "assets/Assets.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr std::uint32_t FnvOffsetBasis = 2166136261u;
    constexpr std::uint32_t FnvPrime = 16777619u;

    std::uint32_t HashTheme(std::string_view theme)
    {
        std::uint32_t hash = FnvOffsetBasis;
        for (const unsigned char character : theme)
        {
            hash ^= character;
            hash *= FnvPrime;
        }
        return hash;
    }

}

GameplayBackground::GameplayBackground(Assets& assets, sf::Vector2f logicalSize)
    : assets(assets)
    , logicalSize(logicalSize)
{
    SetTheme("blue_nebula_region");
}

void GameplayBackground::SetTheme(std::string_view theme, float brightness)
{
	constexpr float GlobalBrightnessMultiplier = 1.1f;
	brightness *= GlobalBrightnessMultiplier;
	farBackground.reset();
	std::optional<Config::Texture> backgroundTexture;
	if (theme == "blue_nebula_region")
		backgroundTexture = Config::Texture::GameplayBackgroundBlueRegion;
	else if (theme == "violet_clouds_region")
		backgroundTexture = Config::Texture::GameplayBackgroundVioletRegion;
	else if (theme == "asteroid_belt_region")
		backgroundTexture = Config::Texture::GameplayBackgroundAsteroidRegion;
	else if (theme == "red_storm_region")
		backgroundTexture = Config::Texture::GameplayBackgroundRedRegion;
	else if (theme == "deep_void_region")
		backgroundTexture = Config::Texture::GameplayBackgroundDeepVoidRegion;
	else if (theme == "emerald_aurora_region")
		backgroundTexture = Config::Texture::GameplayBackgroundEmeraldRegion;
	else if (theme == "rose_nursery_region")
		backgroundTexture = Config::Texture::GameplayBackgroundRoseRegion;
	else if (theme == "frozen_expanse_region")
		backgroundTexture = Config::Texture::GameplayBackgroundFrozenRegion;
	else if (theme == "ion_storm_region")
		backgroundTexture = Config::Texture::GameplayBackgroundIonRegion;
	else if (theme == "last_horizon_region")
		backgroundTexture = Config::Texture::GameplayBackgroundLastHorizon;

	if (backgroundTexture)
	{
		sf::Texture& texture{ assets.Textures().Get(*backgroundTexture) };
		farBackground.emplace(texture);
		const sf::Vector2u textureSize{ texture.getSize() };
		const float scale = std::max(
			logicalSize.x / static_cast<float>(textureSize.x),
			logicalSize.y / static_cast<float>(textureSize.y));
		farBackground->setOrigin({
			static_cast<float>(textureSize.x) * 0.5f,
			static_cast<float>(textureSize.y) * 0.5f });
		farBackground->setPosition(logicalSize * 0.5f);
		farBackground->setScale({ scale, scale });
		const auto channel{ static_cast<std::uint8_t>(
			std::clamp(brightness, 0.f, 1.f) * 255.f) };
		farBackground->setColor(sf::Color(channel, channel, channel));
	}

    if (theme == "violet_clouds_region")
    {
        topColor = { 3, 7, 19 };
        bottomColor = { 35, 16, 63 };
		starColor = { 230, 195, 255 };
    }
    else if (theme == "asteroid_belt_region")
    {
        topColor = { 5, 8, 14 };
        bottomColor = { 33, 29, 29 };
		starColor = { 255, 220, 170 };
    }
    else if (theme == "red_storm_region")
    {
        topColor = { 8, 5, 16 };
        bottomColor = { 55, 12, 27 };
		starColor = { 255, 190, 180 };
    }
    else if (theme == "deep_void_region")
    {
        topColor = { 1, 3, 10 };
        bottomColor = { 5, 11, 23 };
		starColor = { 175, 255, 235 };
	}
	else if (theme == "emerald_aurora_region")
	{
		topColor = { 1, 12, 8 };
		bottomColor = { 6, 40, 20 };
		starColor = { 185, 255, 205 };
	}
	else if (theme == "rose_nursery_region")
	{
		topColor = { 15, 3, 10 };
		bottomColor = { 55, 10, 30 };
		starColor = { 255, 190, 215 };
	}
	else if (theme == "frozen_expanse_region")
	{
		topColor = { 4, 6, 11 };
		bottomColor = { 24, 29, 38 };
		starColor = { 230, 240, 255 };
	}
	else if (theme == "ion_storm_region")
	{
		topColor = { 10, 11, 2 };
		bottomColor = { 42, 45, 5 };
		starColor = { 240, 255, 160 };
	}
	else if (theme == "last_horizon_region")
	{
		topColor = { 12, 2, 2 };
		bottomColor = { 2, 1, 7 };
		starColor = { 255, 175, 145 };
    }
    else
    {
        topColor = { 2, 7, 19 };
        bottomColor = { 7, 25, 52 };
		starColor = { 185, 220, 255 };
    }

    BuildGradient();
    GenerateStars(HashTheme(theme));
}

void GameplayBackground::Update(float deltaTime)
{
    UpdateLayer(middleStars, deltaTime);
    UpdateLayer(nearDust, deltaTime);
    BuildStarVertices(middleStars, middleStarVertices);
    BuildStarVertices(nearDust, nearDustVertices);
}

void GameplayBackground::BuildGradient()
{
    gradient.clear();
    const sf::Vector2f topLeft{ 0.f, 0.f };
    const sf::Vector2f topRight{ logicalSize.x, 0.f };
    const sf::Vector2f bottomLeft{ 0.f, logicalSize.y };
    const sf::Vector2f bottomRight{ logicalSize };

    gradient.append({ topLeft, topColor });
    gradient.append({ bottomLeft, bottomColor });
    gradient.append({ bottomRight, bottomColor });
    gradient.append({ topLeft, topColor });
    gradient.append({ bottomRight, bottomColor });
    gradient.append({ topRight, topColor });
}

void GameplayBackground::GenerateStars(std::uint32_t seed)
{
    std::mt19937 random(seed);
    std::uniform_real_distribution<float> xDistribution(0.f, logicalSize.x);
    std::uniform_real_distribution<float> yDistribution(0.f, logicalSize.y);
    std::uniform_real_distribution<float> middleSizeDistribution(1.f, 2.4f);
    std::uniform_real_distribution<float> nearSizeDistribution(1.8f, 4.f);
    std::uniform_real_distribution<float> middleSpeedDistribution(2.f, 8.f);
    std::uniform_real_distribution<float> nearSpeedDistribution(12.f, 28.f);
    std::uniform_int_distribution<int> middleAlphaDistribution(80, 175);
    std::uniform_int_distribution<int> nearAlphaDistribution(55, 130);

    const auto createStar{ [&](float size, float speed, int alpha)
    {
        return Star{
            { xDistribution(random), yDistribution(random) },
            size,
            speed,
			{ starColor.r, starColor.g, starColor.b,
				static_cast<std::uint8_t>(alpha) }
        };
    } };

    middleStars.clear();
    middleStars.reserve(180);
    for (int i = 0; i < 180; ++i)
    {
        middleStars.push_back(createStar(
            middleSizeDistribution(random),
            middleSpeedDistribution(random),
            middleAlphaDistribution(random)));
    }

    nearDust.clear();
    nearDust.reserve(55);
    for (int i = 0; i < 55; ++i)
    {
        nearDust.push_back(createStar(
            nearSizeDistribution(random),
            nearSpeedDistribution(random),
            nearAlphaDistribution(random)));
    }

    BuildStarVertices(middleStars, middleStarVertices);
    BuildStarVertices(nearDust, nearDustVertices);
}

void GameplayBackground::UpdateLayer(std::vector<Star>& layer, float deltaTime)
{
    for (Star& star : layer)
    {
        star.position.x -= star.speed * 0.08f * deltaTime;
        star.position.y += star.speed * deltaTime;

        if (star.position.y > logicalSize.y + star.size)
            star.position.y = -star.size;
        if (star.position.x < -star.size)
            star.position.x = logicalSize.x + star.size;
    }
}

void GameplayBackground::BuildStarVertices(
    const std::vector<Star>& layer,
    sf::VertexArray& vertices) const
{
    vertices.clear();
    for (const Star& star : layer)
        AppendStarQuad(vertices, star);
}

void GameplayBackground::AppendStarQuad(sf::VertexArray& vertices, const Star& star) const
{
    const float halfSize = star.size * 0.5f;
    const sf::Vector2f topLeft{ star.position.x - halfSize, star.position.y - halfSize };
    const sf::Vector2f topRight{ star.position.x + halfSize, star.position.y - halfSize };
    const sf::Vector2f bottomLeft{ star.position.x - halfSize, star.position.y + halfSize };
    const sf::Vector2f bottomRight{ star.position.x + halfSize, star.position.y + halfSize };

    vertices.append({ topLeft, star.color });
    vertices.append({ bottomLeft, star.color });
    vertices.append({ bottomRight, star.color });
    vertices.append({ topLeft, star.color });
    vertices.append({ bottomRight, star.color });
    vertices.append({ topRight, star.color });
}

void GameplayBackground::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(gradient, states);
    if (farBackground)
        target.draw(*farBackground, states);
    target.draw(middleStarVertices, states);
    target.draw(nearDustVertices, states);
}
