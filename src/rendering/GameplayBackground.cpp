#include "GameplayBackground.h"

#include <algorithm>
#include <random>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr std::uint32_t FnvOffsetBasis{ 2166136261u };
    constexpr std::uint32_t FnvPrime{ 16777619u };

    std::uint32_t HashTheme(std::string_view theme)
    {
        std::uint32_t hash{ FnvOffsetBasis };
        for (const unsigned char character : theme)
        {
            hash ^= character;
            hash *= FnvPrime;
        }
        return hash;
    }

}

GameplayBackground::GameplayBackground(AssetStore& assets, sf::Vector2f logicalSize)
    : assets(assets)
    , logicalSize(logicalSize)
{
    SetTheme("level_01_blue_nebula");
}

void GameplayBackground::SetTheme(std::string_view theme, float brightness)
{
	farBackground.reset();
	std::optional<Config::Texture> backgroundTexture;
	if (theme == "level_01_blue_nebula")
		backgroundTexture = Config::Texture::GameplayBackgroundLevel1;
	else if (theme == "level_02_violet_clouds")
		backgroundTexture = Config::Texture::GameplayBackgroundLevel2;
	else if (theme == "level_03_asteroid_belt")
		backgroundTexture = Config::Texture::GameplayBackgroundLevel3;
	else if (theme == "level_04_red_storm")
		backgroundTexture = Config::Texture::GameplayBackgroundLevel4;
	else if (theme == "level_05_deep_void")
		backgroundTexture = Config::Texture::GameplayBackgroundLevel5;

	if (backgroundTexture)
	{
		sf::Texture& texture{ assets.Textures().Get(*backgroundTexture) };
		farBackground.emplace(texture);
		const sf::Vector2u textureSize{ texture.getSize() };
		const float scale{ std::max(
			logicalSize.x / static_cast<float>(textureSize.x),
			logicalSize.y / static_cast<float>(textureSize.y)) };
		farBackground->setOrigin({
			static_cast<float>(textureSize.x) * 0.5f,
			static_cast<float>(textureSize.y) * 0.5f });
		farBackground->setPosition(logicalSize * 0.5f);
		farBackground->setScale({ scale, scale });
		const auto channel{ static_cast<std::uint8_t>(
			std::clamp(brightness, 0.f, 1.f) * 255.f) };
		farBackground->setColor(sf::Color(channel, channel, channel));
	}

    if (theme == "level_02_violet_clouds")
    {
        topColor = { 3, 7, 19 };
        bottomColor = { 35, 16, 63 };
    }
    else if (theme == "level_03_asteroid_belt")
    {
        topColor = { 5, 8, 14 };
        bottomColor = { 33, 29, 29 };
    }
    else if (theme == "level_04_red_storm")
    {
        topColor = { 8, 5, 16 };
        bottomColor = { 55, 12, 27 };
    }
    else if (theme == "level_05_deep_void")
    {
        topColor = { 1, 3, 10 };
        bottomColor = { 5, 11, 23 };
    }
    else
    {
        topColor = { 2, 7, 19 };
        bottomColor = { 7, 25, 52 };
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
            { 185, 220, 255, static_cast<std::uint8_t>(alpha) }
        };
    } };

    middleStars.clear();
    middleStars.reserve(180);
    for (int i{ 0 }; i < 180; ++i)
    {
        middleStars.push_back(createStar(
            middleSizeDistribution(random),
            middleSpeedDistribution(random),
            middleAlphaDistribution(random)));
    }

    nearDust.clear();
    nearDust.reserve(55);
    for (int i{ 0 }; i < 55; ++i)
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
    const float halfSize{ star.size * 0.5f };
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
