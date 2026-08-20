#include "WaveIntro.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr float SlideDuration{ 0.45f };
    constexpr float TitleHoldDuration{ 0.55f };
    constexpr float FadeDuration{ 0.30f };
    constexpr float TotalDuration{ SlideDuration + TitleHoldDuration + FadeDuration };
    constexpr float TitleTargetY{ 540.f };
    constexpr sf::Color Cyan{ 25, 220, 255 };
    float SmoothStep(float value)
    {
        value = std::clamp(value, 0.f, 1.f);
        return value * value * (3.f - 2.f * value);
    }
}

WaveIntro::WaveIntro(AssetStore& assets)
    : titleGlow(assets)
    , title(assets.Fonts().Get(Config::Font::MenuSemibold), "", 76)
{
    title.setOutlineColor(sf::Color(2, 12, 24, 235));
    title.setOutlineThickness(4.f);
    title.setLetterSpacing(1.1f);
}

void WaveIntro::Start(int waveNumber, bool finalWave)
{
    title.setString(finalWave
        ? "FINAL WAVE"
        : "WAVE " + std::to_string(std::max(1, waveNumber)));
    elapsed = 0.f;
    active = true;
    titleGlow.Invalidate();
    ApplyAnimation();
}

bool WaveIntro::Update(float deltaTime)
{
    if (!active)
        return false;

    titleGlow.Update(deltaTime);
    elapsed = std::min(TotalDuration, elapsed + deltaTime);
    ApplyAnimation();
    if (elapsed < TotalDuration)
        return false;

    active = false;
    return true;
}

void WaveIntro::Draw(sf::RenderTarget& target)
{
    if (!active)
        return;

    titleGlow.DrawBloom(
        target,
        title.getGlobalBounds(),
        [this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
        {
            glowTarget.draw(title, states);
        },
        Cyan);
    target.draw(title);
    titleGlow.DrawHighlight(target, title.getGlobalBounds(), Cyan);
}

bool WaveIntro::IsActive() const noexcept
{
    return active;
}

void WaveIntro::ApplyAnimation()
{
    const float slideProgress{ SmoothStep(elapsed / SlideDuration) };
    const float titleY{ -90.f + (TitleTargetY + 90.f) * slideProgress };
    const float fadeProgress{ elapsed <= SlideDuration + TitleHoldDuration
        ? 0.f
        : SmoothStep((elapsed - SlideDuration - TitleHoldDuration) / FadeDuration) };
    const auto alpha{ static_cast<std::uint8_t>((1.f - fadeProgress) * 255.f) };
    title.setFillColor(sf::Color(215, 247, 252, alpha));
    title.setOutlineColor(sf::Color(2, 12, 24, alpha));
    title.setScale({ 1.f + fadeProgress * 0.06f, 1.f + fadeProgress * 0.06f });
    CenterText(title, { 960.f, titleY });
}

void WaveIntro::CenterText(sf::Text& text, sf::Vector2f position)
{
    const sf::FloatRect bounds{ text.getLocalBounds() };
    text.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });
    text.setPosition(position);
}
