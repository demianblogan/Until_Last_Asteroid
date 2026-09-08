#include "GameplayPostProcessor.h"

#include <algorithm>
#include <array>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>

#include "assets/Assets.h"
#include "rendering/RenderTargetUtils.h"
#include "utils/ConfigEnums.h"

namespace Rendering
{
	namespace
	{
		constexpr float BloomThreshold = 0.79f;
		constexpr float BloomSoftness = 0.14f;
		constexpr float BlurRadius = 2.15f;
		constexpr unsigned int BlurIterations = 2u;
	}

	GameplayPostProcessor::GameplayPostProcessor(Assets& assets, sf::Vector2f logicalSize)
		: logicalSize(logicalSize)
		, brightPassShader(assets.GetShader(Config::Shader::SceneBrightPass))
		, blurShader(assets.GetShader(Config::Shader::GaussianBlur))
		, compositeShader(assets.GetShader(Config::Shader::SceneComposite))
	{}

	void GameplayPostProcessor::Render(sf::RenderWindow& window, const GameplayData::LevelConfig::PostProcessConfig& config,
		const GameplayEffects::PostProcessState& effects, float timeSlowdownStrength, const SceneRenderer& renderScene)
	{
		if (!EnsureSize(window))
		{
			renderScene(window);
			return;
		}

		scene.clear(sf::Color::Black);

		renderScene(scene);

		scene.display();

		// Bloom is a 5-pass, half-resolution effect (bright-pass + two blur
		// iterations). Levels with bloomIntensity at ~0 don't need it redone
		// every frame -- clear the (now unused) bloom texture once and reuse
		// that empty result until bloom is actually needed again.
		constexpr float MinimumBloomIntensity = 0.01f;
		if (config.bloomIntensity > MinimumBloomIntensity)
		{
			ApplyBloom(scene.getTexture());
			isBloomTextureCleared = false;
		}
		else if (!isBloomTextureCleared)
		{
			bloom.clear(sf::Color::Transparent);
			bloom.display();
			isBloomTextureCleared = true;
		}

		compositeShader.setUniform("source", sf::Shader::CurrentTexture);
		compositeShader.setUniform("bloomTexture", bloom.getTexture());
		compositeShader.setUniform("tint", sf::Glsl::Vec3(config.tint[0], config.tint[1], config.tint[2]));
		compositeShader.setUniform("saturation", config.saturation);
		compositeShader.setUniform("contrast", config.contrast);
		compositeShader.setUniform("bloomIntensity", config.bloomIntensity);
		compositeShader.setUniform("vignetteStrength", config.vignetteStrength);
		compositeShader.setUniform("damageVignette", effects.damageVignette);
		compositeShader.setUniform("aspectRatio", logicalSize.x / logicalSize.y);
		const size_t shockwaveCount = std::min(effects.shockwaveCount, GameplayEffects::MaximumShockwaves);
		compositeShader.setUniform("shockwaveCount", static_cast<int>(shockwaveCount));

		if (shockwaveCount > 0u)
		{
			std::array<sf::Glsl::Vec2, GameplayEffects::MaximumShockwaves> centers;
			std::array<float, GameplayEffects::MaximumShockwaves> radii{};
			for (std::size_t index = 0u; index < shockwaveCount; index++)
			{
				// RenderTexture sampling uses a vertically flipped texture matrix.
				// World coordinates originate at the top-left, so convert Y to the
				// shader's bottom-left UV space before locating the shockwave.
				centers[index] = sf::Glsl::Vec2(
					effects.shockwavePositions[index].x / logicalSize.x,
					1.f - effects.shockwavePositions[index].y / logicalSize.y);

				radii[index] = effects.shockwaveRadii[index] / logicalSize.y;
			}

			compositeShader.setUniformArray("shockwaveCenters", centers.data(), shockwaveCount);
			compositeShader.setUniformArray("shockwaveRadii", radii.data(), shockwaveCount);
			compositeShader.setUniformArray("shockwaveStrengths", effects.shockwaveStrengths.data(), shockwaveCount);
		}

		compositeShader.setUniform("timeSlowdownStrength", std::clamp(timeSlowdownStrength, 0.f, 1.f));

		sf::Sprite result(scene.getTexture());
		const sf::Vector2u sceneSize{ scene.getSize() };
		result.setScale({ logicalSize.x / static_cast<float>(sceneSize.x),	logicalSize.y / static_cast<float>(sceneSize.y) });

		sf::RenderStates states;
		states.shader = &compositeShader;
		states.blendMode = sf::BlendNone;

		window.draw(result, states);
	}

	bool GameplayPostProcessor::EnsureSize(sf::RenderWindow& window)
	{
		const sf::Vector2u sceneSize{ GetViewportSize(window) };
		const sf::Vector2u bloomSize{ std::max(1u, sceneSize.x / 2u), std::max(1u, sceneSize.y / 2u) 
		};
		if ((scene.getSize() != sceneSize && !scene.resize(sceneSize)) ||
			(bright.getSize() != bloomSize && !bright.resize(bloomSize)) ||
			(horizontalBlur.getSize() != bloomSize && !horizontalBlur.resize(bloomSize)) ||
			(bloom.getSize() != bloomSize && !bloom.resize(bloomSize)))
		{
			return false;
		}

		scene.setSmooth(true);
		bright.setSmooth(true);
		horizontalBlur.setSmooth(true);
		bloom.setSmooth(true);

		scene.setView(sf::View(sf::FloatRect({ 0.f, 0.f }, logicalSize)));

		return true;
	}

	void GameplayPostProcessor::ApplyBloom(const sf::Texture& source)
	{
		const sf::Vector2u sourceSize{ source.getSize() };
		const sf::Vector2u bloomSize{ bright.getSize() };

		sf::Sprite sourceSprite(source);
		sourceSprite.setScale({
			static_cast<float>(bloomSize.x) / static_cast<float>(sourceSize.x),
			static_cast<float>(bloomSize.y) / static_cast<float>(sourceSize.y)
			});

		brightPassShader.setUniform("source", sf::Shader::CurrentTexture);
		brightPassShader.setUniform("threshold", BloomThreshold);
		brightPassShader.setUniform("softness", BloomSoftness);

		sf::RenderStates brightStates;
		brightStates.shader = &brightPassShader;
		brightStates.blendMode = sf::BlendNone;

		bright.clear(sf::Color::Transparent);

		bright.draw(sourceSprite, brightStates);

		bright.display();

		const sf::Texture* current = &bright.getTexture();
		sf::RenderStates blurStates;
		blurStates.blendMode = sf::BlendNone;

		for (unsigned int iteration = 0u; iteration < BlurIterations; iteration++)
		{
			DrawGaussianBlurPass(horizontalBlur, sf::Sprite(*current), blurShader,
				{ BlurRadius / static_cast<float>(bloomSize.x), 0.f }, blurStates);

			DrawGaussianBlurPass(bloom, sf::Sprite(horizontalBlur.getTexture()), blurShader,
				{ 0.f, BlurRadius / static_cast<float>(bloomSize.y) }, blurStates);

			current = &bloom.getTexture();
		}
	}
}