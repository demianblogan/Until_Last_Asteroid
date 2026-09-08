#include "MenuBackground.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Angle.hpp>

#include "assets/Assets.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"

namespace UI
{
	namespace
	{
		constexpr std::size_t DecorativeStarCount = 90;

		// Number of hand-placed decorative asteroids (see InitializeAsteroids) --
		// every array there has exactly this many entries, one per asteroid.
		constexpr std::size_t DecorativeAsteroidCount = 4;

		// How far past the screen edge a star/asteroid must travel before it wraps
		// around to the opposite side -- keeps the teleport hidden off-screen instead
		// of visibly popping at the exact edge.
		constexpr float OffscreenWrapMargin = 180.f;

		// The background is scaled up an extra 4% beyond exactly covering the
		// screen, so the few pixels of mouse-parallax shift applied to it in Draw
		// (currentParallax * 0.25) never reveal empty space at the screen edges.
		constexpr float BackgroundOverscanFactor = 1.04f;

		// How strongly the screen edges are darkened by the vignette shader.
		constexpr float VignetteStrength = 0.36f;

		// Maximum parallax shift (in pixels) when the mouse is at the screen's
		// edge, along each axis. Not derived from the screen's aspect ratio --
		// just hand-tuned so the horizontal shift reads as slightly stronger
		// than the vertical one.
		constexpr float MaxHorizontalParallaxOffset = 18.f;
		constexpr float MaxVerticalParallaxOffset = 12.f;

		// How quickly currentParallax catches up to targetParallax; see the
		// comment in Update() for what this feeds into.
		constexpr float ParallaxSmoothingRate = 5.f;

		// Stars drift diagonally down-and-right; their vertical speed is this
		// fraction of their horizontal speed (moveSpeed), not a separate value.
		constexpr float StarVerticalDriftRatio = 0.12f;

		// The background's own parallax depth (see DecorativeStar::depth for what
		// depth means) -- smaller than any star or asteroid's, since the
		// background is meant to read as the furthest-away layer.
		constexpr float BackgroundParallaxDepth = 0.25f;
	}

	MenuBackground::DecorativeAsteroid::DecorativeAsteroid(const sf::Texture& texture, float asteroidDepth)
		: sprite(texture)
		, depth(asteroidDepth)
	{}

	MenuBackground::MenuBackground(Assets& assets, sf::Vector2f size)
		: background(assets.Textures().Get(Config::Texture::MainMenuBackground))
		, vignette(size)
		, vignetteShader(assets.GetShader(Config::Shader::MenuVignette))
		, logicalSize(size)
	{
		const sf::Vector2u textureSize = background.getTexture().getSize();

		// Scale by whichever axis needs more enlargement to fully cover the
		// screen (max, not min -- min would leave the other axis under-covered).
		const float backgroundScale = std::max(
			logicalSize.x * BackgroundOverscanFactor / static_cast<float>(textureSize.x),
			logicalSize.y * BackgroundOverscanFactor / static_cast<float>(textureSize.y));

		background.setOrigin({
			static_cast<float>(textureSize.x) * 0.5f,
			static_cast<float>(textureSize.y) * 0.5f
			});
		background.setScale({ backgroundScale, backgroundScale });
		background.setPosition(logicalSize * 0.5f);

		vignette.setPosition({ 0.f, 0.f });
		vignette.setFillColor(sf::Color::White);
		vignetteShader.setUniform("aspectRatio", logicalSize.x / logicalSize.y);
		vignetteShader.setUniform("strength", VignetteStrength);

		InitializeStars();
		InitializeAsteroids(assets);
	}

	void MenuBackground::SetMousePosition(sf::Vector2f position)
	{
		const sf::Vector2f halfSize = logicalSize * 0.5f;

		// The mouse position relative to screen center, scaled to [-1, 1] on each axis.
		const sf::Vector2f normalizedMouseOffset =
		{
			std::clamp((position.x - halfSize.x) / halfSize.x, -1.f, 1.f),
			std::clamp((position.y - halfSize.y) / halfSize.y, -1.f, 1.f)
		};

		// Negated so the background appears to shift toward the mouse, as if the
		// camera turned to look where the player is pointing.
		targetParallax =
		{
			-normalizedMouseOffset.x * MaxHorizontalParallaxOffset,
			-normalizedMouseOffset.y * MaxVerticalParallaxOffset
		};
	}

	void MenuBackground::Update(float deltaTime)
	{
		// Exponential smoothing: currentParallax closes a fraction of the
		// remaining distance to targetParallax every frame, so it eases toward
		// the target instead of snapping straight to it. The fraction itself is
		// derived from deltaTime (via std::exp), which is what keeps the motion
		// speed the same regardless of framerate, rather than moving a fixed step
		// per frame.
		const float smoothing = 1.f - std::exp(-ParallaxSmoothingRate * deltaTime);
		currentParallax += (targetParallax - currentParallax) * smoothing;

		UpdateStars(deltaTime);
		UpdateAsteroids(deltaTime);
	}

	void MenuBackground::UpdateStars(float deltaTime)
	{
		for (DecorativeStar& star : stars)
		{
			star.shape.move({ -star.moveSpeed * deltaTime, star.moveSpeed * StarVerticalDriftRatio * deltaTime });

			sf::Vector2f position = star.shape.getPosition();

			if (position.x < -OffscreenWrapMargin)
				position.x = logicalSize.x + OffscreenWrapMargin;

			if (position.y > logicalSize.y + OffscreenWrapMargin)
				position.y = -OffscreenWrapMargin;

			star.shape.setPosition(position);
		}
	}

	void MenuBackground::UpdateAsteroids(float deltaTime)
	{
		for (DecorativeAsteroid& asteroid : asteroids)
		{
			asteroid.sprite.move(asteroid.velocity * deltaTime);
			asteroid.sprite.rotate(sf::degrees(asteroid.rotationSpeed * deltaTime));

			sf::Vector2f position = asteroid.sprite.getPosition();

			if (position.x < -OffscreenWrapMargin)
				position.x = logicalSize.x + OffscreenWrapMargin;
			else if (position.x > logicalSize.x + OffscreenWrapMargin)
				position.x = -OffscreenWrapMargin;

			if (position.y < -OffscreenWrapMargin)
				position.y = logicalSize.y + OffscreenWrapMargin;
			else if (position.y > logicalSize.y + OffscreenWrapMargin)
				position.y = -OffscreenWrapMargin;

			asteroid.sprite.setPosition(position);
		}
	}

	void MenuBackground::Draw(sf::RenderTarget& target) const
	{
		DrawBackground(target);
		DrawStars(target);
		DrawAsteroids(target);
		DrawVignette(target);
	}

	void MenuBackground::DrawBackground(sf::RenderTarget& target) const
	{
		sf::RenderStates states;
		states.transform.translate(currentParallax * BackgroundParallaxDepth);
		target.draw(background, states);
	}

	void MenuBackground::DrawStars(sf::RenderTarget& target) const
	{
		sf::RenderStates states;
		for (const DecorativeStar& star : stars)
		{
			// Reset to Identity first: translate() composes with whatever
			// transform is already there, so without resetting, each star's
			// offset would stack on top of the previous star's.
			states.transform = sf::Transform::Identity;
			states.transform.translate(currentParallax * star.depth);

			target.draw(star.shape, states);
		}
	}

	void MenuBackground::DrawAsteroids(sf::RenderTarget& target) const
	{
		sf::RenderStates states;
		for (const DecorativeAsteroid& asteroid : asteroids)
		{
			states.transform = sf::Transform::Identity;
			states.transform.translate(currentParallax * asteroid.depth);

			target.draw(asteroid.sprite, states);
		}
	}

	void MenuBackground::DrawVignette(sf::RenderTarget& target) const
	{
		// The visible drawing area can be smaller than the full render target
		// (letterboxing -- see DisplayManager::RestoreLogicalView), so the
		// vignette shader needs to know exactly where that area is and how big it
		// is, or it would darken relative to the wrong bounds (e.g. including the
		// letterbox bars).
		const sf::IntRect viewport = target.getViewport(target.getView());
		const sf::Vector2u targetSize = target.getSize();

		// SFML measures viewport.position.y from the top of the target, but the
		// shader (GLSL/OpenGL convention) expects Y measured from the bottom, so
		// it's flipped here before being sent over.
		const float viewportBottom = static_cast<float>(targetSize.y) - static_cast<float>(viewport.position.y + viewport.size.y);

		vignetteShader.setUniform("viewportOrigin", sf::Glsl::Vec2(static_cast<float>(viewport.position.x), viewportBottom));
		vignetteShader.setUniform("viewportSize", sf::Glsl::Vec2(
			static_cast<float>(std::max(1, viewport.size.x)),
			static_cast<float>(std::max(1, viewport.size.y))));

		sf::RenderStates vignetteStates;
		vignetteStates.shader = &vignetteShader;

		target.draw(vignette, vignetteStates);
	}

	void MenuBackground::InitializeStars()
	{
		stars.reserve(DecorativeStarCount);

		for (std::size_t index = 0u; index < DecorativeStarCount; index++)
		{
			const float depth = Random::Float(0.25f, 1.f);
			const float radius = Random::Float(0.6f, 1.8f) * depth;
			DecorativeStar star =
			{
				sf::CircleShape(radius),
				5.f + depth * 14.f,
				depth
			};

			star.shape.setOrigin({ radius, radius });
			star.shape.setPosition({ Random::Float(0.f, logicalSize.x), Random::Float(0.f, logicalSize.y) });
			star.shape.setFillColor(sf::Color(
				static_cast<std::uint8_t>(155.f + 70.f * depth),
				static_cast<std::uint8_t>(190.f + 55.f * depth),
				255,
				static_cast<std::uint8_t>(55.f + 105.f * depth)));

			stars.push_back(std::move(star));
		}
	}

	void MenuBackground::InitializeAsteroids(Assets& assets)
	{
		using Config::Texture;

		const std::array<Texture, DecorativeAsteroidCount> textures =
		{
			Texture::BigMeteor1,
			Texture::BigMeteor3,
			Texture::SmallMeteor1,
			Texture::SmallMeteor2
		};

		const std::array<sf::Vector2f, DecorativeAsteroidCount> positions =
		{
			sf::Vector2f{ 1520.f, 150.f },
			sf::Vector2f{ 1180.f, 890.f },
			sf::Vector2f{ 720.f, 240.f },
			sf::Vector2f{ 1760.f, 720.f }
		};

		const std::array<sf::Vector2f, DecorativeAsteroidCount> velocities =
		{
			sf::Vector2f{ -8.f, 2.f },
			sf::Vector2f{ 5.f, -3.f },
			sf::Vector2f{ -4.f, 1.f },
			sf::Vector2f{ 7.f, 3.f }
		};

		const std::array<float, DecorativeAsteroidCount> scales = { 0.8f, 0.62f, 0.48f, 0.7f };
		const std::array<float, DecorativeAsteroidCount> depths = { 0.65f, 0.5f, 0.35f, 0.8f };

		asteroids.reserve(textures.size());

		for (std::size_t index = 0u; index < textures.size(); index++)
		{
			const sf::Texture& texture = assets.Textures().Get(textures[index]);
			DecorativeAsteroid asteroid(texture, depths[index]);
			const sf::Vector2u textureSize = texture.getSize();

			asteroid.sprite.setOrigin({ static_cast<float>(textureSize.x) * 0.5f, static_cast<float>(textureSize.y) * 0.5f });
			asteroid.sprite.setPosition(positions[index]);
			asteroid.sprite.setScale({ scales[index], scales[index] });
			asteroid.sprite.setColor(sf::Color(160, 190, 215, static_cast<std::uint8_t>(35.f + depths[index] * 45.f)));
			asteroid.velocity = velocities[index];
			asteroid.rotationSpeed = index % 2 == 0 ? 3.5f + static_cast<float>(index) : -2.5f - static_cast<float>(index);

			asteroids.push_back(std::move(asteroid));
		}
	}
}