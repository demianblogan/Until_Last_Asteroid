#pragma once

#include <functional>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Vector2.hpp>

class AssetStore;

namespace sf
{
    class RenderTarget;
    class Shader;
    class Texture;
}

class NeonGlow
{
public:
    using SourceRenderer = std::function<void(sf::RenderTarget&, const sf::RenderStates&)>;

    explicit NeonGlow(AssetStore& assets);

    void Update(float deltaTime);
    void Invalidate() noexcept;
    void DrawBloom(
        sf::RenderTarget& target,
        const sf::FloatRect& bounds,
        const SourceRenderer& renderSource,
        sf::Color color);
    void DrawHighlight(
        sf::RenderTarget& target,
        const sf::FloatRect& bounds,
        sf::Color color) const;

private:
    void Rebuild(const sf::FloatRect& bounds, const SourceRenderer& renderSource);
    [[nodiscard]] bool Resize(sf::Vector2f contentSize);
    void ApplyBlur(
        const sf::Texture& input,
        sf::RenderTexture& output,
        float radius,
        unsigned int iterations);
    [[nodiscard]] float GetPulse() const;

    sf::RenderTexture source;
    sf::RenderTexture emissive;
    sf::RenderTexture horizontalBlur;
    sf::RenderTexture innerBlur;
    sf::RenderTexture outerBlur;
    sf::Shader& brightPassShader;
    sf::Shader& blurShader;
    sf::Vector2f cachedContentSize;
    float elapsedTime{ 0.f };
    bool dirty{ true };
};
