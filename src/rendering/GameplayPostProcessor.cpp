#include "GameplayPostProcessor.h"

#include <algorithm>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/View.hpp>

#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr float BloomThreshold{ 0.79f };
    constexpr float BloomSoftness{ 0.14f };
    constexpr float BlurRadius{ 2.15f };
    constexpr unsigned int BlurIterations{ 2u };

    sf::Vector2u GetViewportSize(sf::RenderWindow& window)
    {
        const sf::IntRect viewport{ window.getViewport(window.getView()) };
        if (viewport.size.x <= 0 || viewport.size.y <= 0)
        {
            const sf::Vector2u windowSize{ window.getSize() };
            return { std::max(1u, windowSize.x), std::max(1u, windowSize.y) };
        }
        return {
            static_cast<unsigned int>(viewport.size.x),
            static_cast<unsigned int>(viewport.size.y)
        };
    }
}

GameplayPostProcessor::GameplayPostProcessor(AssetStore& assets, sf::Vector2f logicalSize)
    : logicalSize(logicalSize)
    , brightPassShader(assets.GetShader(Config::Shader::SceneBrightPass))
    , blurShader(assets.GetShader(Config::Shader::GaussianBlur))
    , compositeShader(assets.GetShader(Config::Shader::SceneComposite))
{
}

void GameplayPostProcessor::Render(
    sf::RenderWindow& window,
    const GameplayData::LevelConfig::PostProcessConfig& config,
    const GameplayEffects::PostProcessState& effects,
    const SceneRenderer& renderScene)
{
    if (!EnsureSize(window))
    {
        renderScene(window);
        return;
    }

    scene.clear(sf::Color::Black);
    renderScene(scene);
    scene.display();

    ApplyBloom(scene.getTexture());

    compositeShader.setUniform("source", sf::Shader::CurrentTexture);
    compositeShader.setUniform("bloomTexture", bloom.getTexture());
    compositeShader.setUniform("tint", sf::Glsl::Vec3(
        config.tint[0], config.tint[1], config.tint[2]));
    compositeShader.setUniform("saturation", config.saturation);
    compositeShader.setUniform("contrast", config.contrast);
    compositeShader.setUniform("bloomIntensity", config.bloomIntensity);
    compositeShader.setUniform("vignetteStrength", config.vignetteStrength);
    compositeShader.setUniform("damageVignette", effects.damageVignette);
    compositeShader.setUniform("aspectRatio", logicalSize.x / logicalSize.y);
    compositeShader.setUniform("shockwaveActive", effects.shockwaveActive);
    compositeShader.setUniform("shockwaveCenter", sf::Glsl::Vec2(
        effects.shockwavePosition.x / logicalSize.x,
        effects.shockwavePosition.y / logicalSize.y));
    compositeShader.setUniform("shockwaveRadius", effects.shockwaveRadius / logicalSize.y);
    compositeShader.setUniform("shockwaveStrength", effects.shockwaveStrength);

    sf::Sprite result(scene.getTexture());
    const sf::Vector2u sceneSize{ scene.getSize() };
    result.setScale({
        logicalSize.x / static_cast<float>(sceneSize.x),
        logicalSize.y / static_cast<float>(sceneSize.y)
    });
    sf::RenderStates states;
    states.shader = &compositeShader;
    states.blendMode = sf::BlendNone;
    window.draw(result, states);
}

bool GameplayPostProcessor::EnsureSize(sf::RenderWindow& window)
{
    const sf::Vector2u sceneSize{ GetViewportSize(window) };
    const sf::Vector2u bloomSize{
        std::max(1u, sceneSize.x / 2u),
        std::max(1u, sceneSize.y / 2u)
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

    const sf::Texture* current{ &bright.getTexture() };
    sf::RenderStates blurStates;
    blurStates.shader = &blurShader;
    blurStates.blendMode = sf::BlendNone;
    for (unsigned int iteration{ 0u }; iteration < BlurIterations; ++iteration)
    {
        blurShader.setUniform("source", sf::Shader::CurrentTexture);
        blurShader.setUniform("direction", sf::Glsl::Vec2(
            BlurRadius / static_cast<float>(bloomSize.x), 0.f));
        horizontalBlur.clear(sf::Color::Transparent);
        horizontalBlur.draw(sf::Sprite(*current), blurStates);
        horizontalBlur.display();

        blurShader.setUniform("direction", sf::Glsl::Vec2(
            0.f, BlurRadius / static_cast<float>(bloomSize.y)));
        bloom.clear(sf::Color::Transparent);
        bloom.draw(sf::Sprite(horizontalBlur.getTexture()), blurStates);
        bloom.display();
        current = &bloom.getTexture();
    }
}
