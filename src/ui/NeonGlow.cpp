#include "NeonGlow.h"

#include <algorithm>
#include <cstdint>
#include <cmath>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include "assets/AssetStore.h"
#include "ui/RoundedRectangleShape.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr float RenderScale{ 0.5f };
    constexpr float Padding{ 64.f };
    constexpr float CornerRadius{ 16.f };
    constexpr float BlurRadius{ 4.5f };
    constexpr float PulseSpeed{ 3.2f };

    sf::Vector2u ToTextureSize(sf::Vector2f contentSize)
    {
        return {
            std::max(1u, static_cast<unsigned int>((contentSize.x + Padding * 2.f) * RenderScale)),
            std::max(1u, static_cast<unsigned int>((contentSize.y + Padding * 2.f) * RenderScale))
        };
    }
}

NeonGlow::NeonGlow(AssetStore& assets)
    : blurShader(assets.GetShader(Config::Shader::GaussianBlur))
{
}

void NeonGlow::Update(float deltaTime)
{
    elapsedTime = std::fmod(elapsedTime + deltaTime, 1000.f);
}

void NeonGlow::Draw(sf::RenderTarget& target, const sf::FloatRect& bounds)
{
    if (cachedContentSize != bounds.size)
        Rebuild(bounds.size);

    if (blurred.getSize().x == 0u || blurred.getSize().y == 0u)
        return;

    const float pulse{ 0.35f + 0.65f * (std::sin(elapsedTime * PulseSpeed) * 0.5f + 0.5f) };
    sf::Sprite aura(blurred.getTexture());
    aura.setOrigin(sf::Vector2f(blurred.getSize()) * 0.5f);
    aura.setPosition(bounds.position + bounds.size * 0.5f);

    sf::RenderStates additive;
    additive.blendMode = sf::BlendAdd;
    const float baseScale{ 1.f / RenderScale };
    aura.setScale({ baseScale * 1.06f, baseScale * 1.06f });
    aura.setColor(sf::Color(0, 185, 255, static_cast<std::uint8_t>(150.f * pulse)));
    target.draw(aura, additive);

    aura.setScale({ baseScale, baseScale });
    aura.setColor(sf::Color(0, 230, 255, static_cast<std::uint8_t>(255.f * pulse)));
    target.draw(aura, additive);
    aura.setColor(sf::Color(72, 245, 255, static_cast<std::uint8_t>(185.f * pulse)));
    target.draw(aura, additive);

    RoundedRectangleShape rim(bounds.size + sf::Vector2f{ 4.f, 4.f }, CornerRadius + 2.f, 10u);
    rim.setPosition(bounds.position - sf::Vector2f{ 2.f, 2.f });
    rim.setFillColor(sf::Color::Transparent);
    rim.setOutlineColor(sf::Color(105, 250, 255, static_cast<std::uint8_t>(255.f * pulse)));
    rim.setOutlineThickness(2.5f);
    target.draw(rim, additive);
}

void NeonGlow::Rebuild(sf::Vector2f contentSize)
{
    cachedContentSize = contentSize;
    const sf::Vector2u textureSize{ ToTextureSize(contentSize) };
    if (!mask.resize(textureSize) ||
        !horizontalBlur.resize(textureSize) ||
        !blurred.resize(textureSize))
    {
        return;
    }

    mask.setSmooth(true);
    horizontalBlur.setSmooth(true);
    blurred.setSmooth(true);

    mask.clear(sf::Color::Transparent);
    RoundedRectangleShape source(contentSize * RenderScale, CornerRadius * RenderScale, 10u);
    source.setPosition({ Padding * RenderScale, Padding * RenderScale });
    source.setFillColor(sf::Color::White);
    mask.draw(source);
    mask.display();

    blurShader.setUniform("source", sf::Shader::CurrentTexture);
    blurShader.setUniform("direction", sf::Glsl::Vec2(
        BlurRadius / static_cast<float>(textureSize.x), 0.f));
    sf::RenderStates blurStates;
    blurStates.shader = &blurShader;

    horizontalBlur.clear(sf::Color::Transparent);
    horizontalBlur.draw(sf::Sprite(mask.getTexture()), blurStates);
    horizontalBlur.display();

    blurShader.setUniform("direction", sf::Glsl::Vec2(
        0.f, BlurRadius / static_cast<float>(textureSize.y)));
    blurred.clear(sf::Color::Transparent);
    blurred.draw(sf::Sprite(horizontalBlur.getTexture()), blurStates);
    blurred.display();
}
