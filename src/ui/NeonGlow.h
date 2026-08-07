#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Vector2.hpp>

class AssetStore;

namespace sf
{
    class RenderTarget;
    class Shader;
}

class NeonGlow
{
public:
    explicit NeonGlow(AssetStore& assets);

    void Update(float deltaTime);
    void Draw(sf::RenderTarget& target, const sf::FloatRect& bounds);

private:
    void Rebuild(sf::Vector2f contentSize);

    sf::RenderTexture mask;
    sf::RenderTexture horizontalBlur;
    sf::RenderTexture blurred;
    sf::Shader& blurShader;
    sf::Vector2f cachedContentSize;
    float elapsedTime{ 0.f };
};
