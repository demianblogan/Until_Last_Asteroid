#include "GameplayBackground.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Vertex.hpp>

#include "assets/Assets.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"

namespace Rendering
{
	namespace
	{
		// The look of one region theme: which background texture to show behind
		// the stars, and the three colors that drive the sky gradient (topColor/
		// bottomColor) and the star tint (starColor). SetTheme used to pick these
		// out of two separate if/else-if chains keyed by the same theme names --
		// merged into one table here so each theme's data lives in exactly one
		// place instead of two that could quietly drift apart.
		struct ThemePalette
		{
			std::string_view name;
			Config::Texture backgroundTexture;
			sf::Color topColor;
			sf::Color bottomColor;
			sf::Color starColor;
		};

		// The look used both for "blue_nebula_region" and for any theme name
		// SetTheme doesn't recognize -- named here so the table entry below and
		// SetTheme's fallback path read from the same values instead of two
		// copies that happen to match.
		constexpr sf::Color DefaultTopColor{ 2, 7, 19 };
		constexpr sf::Color DefaultBottomColor{ 7, 25, 52 };
		constexpr sf::Color DefaultStarColor{ 185, 220, 255 };

		constexpr std::array<ThemePalette, 10> ThemePalettes =
		{
			{
				{
					"blue_nebula_region",
					Config::Texture::GameplayBackgroundBlueRegion,
					DefaultTopColor, DefaultBottomColor, DefaultStarColor
				},
				{
					"violet_clouds_region",
					Config::Texture::GameplayBackgroundVioletRegion,
					{ 3, 7, 19 }, { 35, 16, 63 }, { 230, 195, 255 }
				},
				{
					"asteroid_belt_region",
					Config::Texture::GameplayBackgroundAsteroidRegion,
					{ 5, 8, 14 }, { 33, 29, 29 }, { 255, 220, 170 }
				},
				{
					"red_storm_region",
					Config::Texture::GameplayBackgroundRedRegion,
					{ 8, 5, 16 }, { 55, 12, 27 }, { 255, 190, 180 }
				},
				{
					"deep_void_region",
					Config::Texture::GameplayBackgroundDeepVoidRegion,
					{ 1, 3, 10 }, { 5, 11, 23 }, { 175, 255, 235 }
				},
				{
					"emerald_aurora_region",
					Config::Texture::GameplayBackgroundEmeraldRegion,
					{ 1, 12, 8 }, { 6, 40, 20 }, { 185, 255, 205 }
				},
				{
					"rose_nursery_region",
					Config::Texture::GameplayBackgroundRoseRegion,
					{ 15, 3, 10 }, { 55, 10, 30 }, { 255, 190, 215 }
				},
				{
					"frozen_expanse_region",
					Config::Texture::GameplayBackgroundFrozenRegion,
					{ 4, 6, 11 }, { 24, 29, 38 }, { 230, 240, 255 }
				},
				{
					"ion_storm_region",
					Config::Texture::GameplayBackgroundIonRegion,
					{ 10, 11, 2 }, { 42, 45, 5 }, { 240, 255, 160 }
				},
				{
					"last_horizon_region",
					Config::Texture::GameplayBackgroundLastHorizon,
					{ 12, 2, 2 }, { 2, 1, 7 }, { 255, 175, 145 }
				},
			}
		};

		const ThemePalette* FindThemePalette(std::string_view theme)
		{
			const auto found = std::ranges::find(ThemePalettes, theme, &ThemePalette::name);
			return found != ThemePalettes.end() ? &*found : nullptr;
		}

		constexpr int MiddleStarCount = 180;
		constexpr int NearDustCount = 55;
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

		const ThemePalette* palette = FindThemePalette(theme);

		if (palette)
		{
			sf::Texture& texture = assets.Textures().Get(palette->backgroundTexture);
			farBackground.emplace(texture);

			const sf::Vector2u textureSize = texture.getSize();
			const float scale = std::max(
				logicalSize.x / static_cast<float>(textureSize.x),
				logicalSize.y / static_cast<float>(textureSize.y));

			farBackground->setOrigin({
				static_cast<float>(textureSize.x) * 0.5f,
				static_cast<float>(textureSize.y) * 0.5f });
			farBackground->setPosition(logicalSize * 0.5f);
			farBackground->setScale({ scale, scale });

			const std::uint8_t channel = static_cast<std::uint8_t>(std::clamp(brightness, 0.f, 1.f) * 255.f);
			farBackground->setColor(sf::Color(channel, channel, channel));

			topColor = palette->topColor;
			bottomColor = palette->bottomColor;
			starColor = palette->starColor;
		}
		else
		{
			// Unrecognized theme name -- no background texture, fall back to
			// the same look as "blue_nebula_region".
			topColor = DefaultTopColor;
			bottomColor = DefaultBottomColor;
			starColor = DefaultStarColor;
		}

		BuildGradient();
		GenerateStars();
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

	void GameplayBackground::GenerateStars()
	{
		const auto createStar = [&](float size, float speed, int alpha)
			{
				return Star{
					{ Random::Float(0.f, logicalSize.x), Random::Float(0.f, logicalSize.y) },
					size,
					speed,
					{ starColor.r, starColor.g, starColor.b, static_cast<std::uint8_t>(alpha) }
				};
			};

		middleStars.clear();
		middleStars.reserve(MiddleStarCount);

		for (int i = 0; i < MiddleStarCount; i++)
		{
			middleStars.push_back(createStar(
				Random::Float(1.f, 2.4f),
				Random::Float(2.f, 8.f),
				Random::Int(80, 175)));
		}

		nearDust.clear();
		nearDust.reserve(NearDustCount);

		for (int i = 0; i < NearDustCount; i++)
		{
			nearDust.push_back(createStar(
				Random::Float(1.8f, 4.f),
				Random::Float(12.f, 28.f),
				Random::Int(55, 130)));
		}

		BuildStarVertices(middleStars, middleStarVertices);
		BuildStarVertices(nearDust, nearDustVertices);
	}

	void GameplayBackground::UpdateLayer(std::vector<Star>& layer, float deltaTime) const
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

	void GameplayBackground::BuildStarVertices(const std::vector<Star>& layer, sf::VertexArray& vertices) const
	{
		vertices.clear();

		for (const Star& star : layer)
			AppendStarQuad(vertices, star);
	}

	void GameplayBackground::AppendStarQuad(sf::VertexArray& vertices, const Star& star) const
	{
		const float StarHalfSize = star.size * 0.5f;

		const sf::Vector2f topLeft{ star.position.x - StarHalfSize, star.position.y - StarHalfSize };
		const sf::Vector2f topRight{ star.position.x + StarHalfSize, star.position.y - StarHalfSize };
		const sf::Vector2f bottomLeft{ star.position.x - StarHalfSize, star.position.y + StarHalfSize };
		const sf::Vector2f bottomRight{ star.position.x + StarHalfSize, star.position.y + StarHalfSize };

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
}