#pragma once

#include <functional>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Vector2.hpp>

#include "game/GameplayData.h"
#include "rendering/GameplayEffects.h"

class AssetStore;

namespace sf
{
    class RenderTarget;
    class RenderWindow;
    class Shader;
    class Texture;
}

class GameplayPostProcessor
{
public:
    using SceneRenderer = std::function<void(sf::RenderTarget&)>;

    GameplayPostProcessor(AssetStore& assets, sf::Vector2f logicalSize);

    void Render(
        sf::RenderWindow& window,
        const GameplayData::LevelConfig::PostProcessConfig& config,
        const GameplayEffects::PostProcessState& effects,
		float timeSlowdownStrength,
        const SceneRenderer& renderScene);

private:
    [[nodiscard]] bool EnsureSize(sf::RenderWindow& window);
    void ApplyBloom(const sf::Texture& source);

    sf::Vector2f logicalSize;
    sf::RenderTexture scene;
    sf::RenderTexture bright;
    sf::RenderTexture horizontalBlur;
    sf::RenderTexture bloom;
    sf::Shader& brightPassShader;
    sf::Shader& blurShader;
    sf::Shader& compositeShader;
};
