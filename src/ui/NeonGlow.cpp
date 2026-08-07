#include "NeonGlow.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr float Padding{ 72.f };
    constexpr float InnerBlurRadius{ 0.9f };
    constexpr float OuterBlurRadius{ 1.5f };
    constexpr unsigned int InnerBlurIterations{ 2u };
    constexpr unsigned int OuterBlurIterations{ 10u };
    constexpr float BrightnessThreshold{ 0.46f };
    constexpr float BrightnessSoftness{ 0.16f };
    constexpr float PulseSpeed{ 3.2f };

    sf::Vector2u ToTextureSize(sf::Vector2f contentSize)
    {
        return {
            std::max(1u, static_cast<unsigned int>(std::ceil(contentSize.x + Padding * 2.f))),
            std::max(1u, static_cast<unsigned int>(std::ceil(contentSize.y + Padding * 2.f)))
        };
    }

    sf::Color ModulatedColor(sf::Color color, float intensity)
    {
        const auto channel{ [intensity](std::uint8_t value)
            {
                return static_cast<std::uint8_t>(std::clamp(
                    static_cast<float>(value) * intensity,
                    0.f,
                    255.f));
            } };
        return { channel(color.r), channel(color.g), channel(color.b), 255u };
    }

    const sf::BlendMode PureAdditive(
        sf::BlendMode::Factor::One,
        sf::BlendMode::Factor::One,
        sf::BlendMode::Equation::Add);
}

NeonGlow::NeonGlow(AssetStore& assets)
    : brightPassShader(assets.GetShader(Config::Shader::BrightPass))
    , blurShader(assets.GetShader(Config::Shader::GaussianBlur))
{
}

void NeonGlow::Update(float deltaTime)
{
    elapsedTime = std::fmod(elapsedTime + deltaTime, 1000.f);
}

void NeonGlow::Invalidate() noexcept
{
    dirty = true;
}

void NeonGlow::DrawBloom(
    sf::RenderTarget& target,
    const sf::FloatRect& bounds,
    const SourceRenderer& renderSource,
    sf::Color color)
{
    if (dirty || cachedContentSize != bounds.size)
        Rebuild(bounds, renderSource);

    if (dirty || outerBlur.getSize().x == 0u || outerBlur.getSize().y == 0u)
        return;

    const float pulse{ GetPulse() };
    const sf::Vector2f position{ bounds.position - sf::Vector2f{ Padding, Padding } };
    sf::RenderStates additive;
    additive.blendMode = PureAdditive;

    sf::Sprite outer(outerBlur.getTexture());
    outer.setPosition(position);
    outer.setColor(ModulatedColor(color, 0.92f * pulse));
    target.draw(outer, additive);
    target.draw(outer, additive);

    sf::Sprite inner(innerBlur.getTexture());
    inner.setPosition(position);
    inner.setColor(ModulatedColor(color, 0.96f * (0.72f + pulse * 0.28f)));
    target.draw(inner, additive);
    target.draw(inner, additive);
}

void NeonGlow::DrawHighlight(
    sf::RenderTarget& target,
    const sf::FloatRect& bounds,
    sf::Color color) const
{
    if (dirty || emissive.getSize().x == 0u || emissive.getSize().y == 0u)
        return;

    const float pulse{ GetPulse() };
    sf::Sprite highlight(emissive.getTexture());
    highlight.setPosition(bounds.position - sf::Vector2f{ Padding, Padding });
    highlight.setColor(ModulatedColor(color, 0.7f * (0.58f + pulse * 0.42f)));

    sf::RenderStates additive;
    additive.blendMode = PureAdditive;
    target.draw(highlight, additive);
}

void NeonGlow::Rebuild(const sf::FloatRect& bounds, const SourceRenderer& renderSource)
{
    if (cachedContentSize != bounds.size && !Resize(bounds.size))
        return;

    source.clear(sf::Color::Transparent);
    sf::RenderStates sourceStates;
    sourceStates.transform.translate(sf::Vector2f{ Padding, Padding } - bounds.position);
    renderSource(source, sourceStates);
    source.display();

    brightPassShader.setUniform("source", sf::Shader::CurrentTexture);
    brightPassShader.setUniform("threshold", BrightnessThreshold);
    brightPassShader.setUniform("softness", BrightnessSoftness);
    sf::RenderStates brightPassStates;
    brightPassStates.shader = &brightPassShader;
    brightPassStates.blendMode = sf::BlendNone;

    emissive.clear(sf::Color::Transparent);
    emissive.draw(sf::Sprite(source.getTexture()), brightPassStates);
    emissive.display();

    ApplyBlur(emissive.getTexture(), innerBlur, InnerBlurRadius, InnerBlurIterations);
    ApplyBlur(emissive.getTexture(), outerBlur, OuterBlurRadius, OuterBlurIterations);
    dirty = false;
}

bool NeonGlow::Resize(sf::Vector2f contentSize)
{
    const sf::Vector2u textureSize{ ToTextureSize(contentSize) };
    if (!source.resize(textureSize) ||
        !emissive.resize(textureSize) ||
        !horizontalBlur.resize(textureSize) ||
        !innerBlur.resize(textureSize) ||
        !outerBlur.resize(textureSize))
    {
        return false;
    }

    cachedContentSize = contentSize;
    source.setSmooth(true);
    emissive.setSmooth(true);
    horizontalBlur.setSmooth(true);
    innerBlur.setSmooth(true);
    outerBlur.setSmooth(true);
    return true;
}

void NeonGlow::ApplyBlur(
    const sf::Texture& input,
    sf::RenderTexture& output,
    float radius,
    unsigned int iterations)
{
    const sf::Vector2u textureSize{ input.getSize() };
    sf::RenderStates blurStates;
    blurStates.shader = &blurShader;
    blurStates.blendMode = sf::BlendNone;
    const sf::Texture* currentInput{ &input };
    for (unsigned int iteration{ 0u }; iteration < iterations; ++iteration)
    {
        blurShader.setUniform("source", sf::Shader::CurrentTexture);
        blurShader.setUniform("direction", sf::Glsl::Vec2(
            radius / static_cast<float>(textureSize.x), 0.f));
        horizontalBlur.clear(sf::Color::Transparent);
        horizontalBlur.draw(sf::Sprite(*currentInput), blurStates);
        horizontalBlur.display();

        blurShader.setUniform("direction", sf::Glsl::Vec2(
            0.f, radius / static_cast<float>(textureSize.y)));
        output.clear(sf::Color::Transparent);
        output.draw(sf::Sprite(horizontalBlur.getTexture()), blurStates);
        output.display();
        currentInput = &output.getTexture();
    }
}

float NeonGlow::GetPulse() const
{
    return 0.58f + 0.42f * (std::sin(elapsedTime * PulseSpeed) * 0.5f + 0.5f);
}
