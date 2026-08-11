#include "WaveIntro.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr float SlideDuration{ 0.45f };
    constexpr float TitleHoldDuration{ 0.35f };
    constexpr std::size_t CountdownStepCount{ 4u };
    constexpr float TitleTargetY{ 390.f };
    constexpr float CountdownY{ 555.f };
    constexpr sf::Color Cyan{ 25, 220, 255 };
    constexpr sf::Color FightColor{ 255, 178, 42 };
    const std::array<std::string, 3> WaveTitles{
        "FIRST WAVE", "SECOND WAVE", "FINAL WAVE"
    };
    const std::array<std::string, CountdownStepCount> Labels{ "3", "2", "1", "GO!" };

    float SmoothStep(float value)
    {
        value = std::clamp(value, 0.f, 1.f);
        return value * value * (3.f - 2.f * value);
    }
}

WaveIntro::WaveIntro(AssetStore& assets, AudioManager& audioManager)
    : titleGlow(assets)
    , countdownGlow(assets)
    , title(assets.Fonts().Get(Config::Font::MenuSemibold), "", 76)
    , countdown(assets.Fonts().Get(Config::Font::MenuSemibold), "", 112)
    , audio(audioManager)
	, countdownStepDuration(
		assets.Sounds().Get(Config::Sound::Countdown).getDuration().asSeconds() /
		static_cast<float>(CountdownStepCount))
	, totalDuration(
		SlideDuration + TitleHoldDuration +
		assets.Sounds().Get(Config::Sound::Countdown).getDuration().asSeconds())
{
    title.setOutlineColor(sf::Color(2, 12, 24, 235));
    title.setOutlineThickness(4.f);
    title.setLetterSpacing(1.1f);
    countdown.setOutlineColor(sf::Color(2, 12, 24, 235));
    countdown.setOutlineThickness(5.f);
    countdown.setLetterSpacing(1.08f);
}

void WaveIntro::Start(int waveNumber)
{
    title.setString(WaveTitles.at(static_cast<std::size_t>(waveNumber - 1)));
    countdown.setString("");
    elapsed = 0.f;
	countdownSoundStarted = false;
    active = true;
    titleGlow.Invalidate();
    countdownGlow.Invalidate();
    ApplyAnimation();
}

bool WaveIntro::Update(float deltaTime)
{
    if (!active)
        return false;

    titleGlow.Update(deltaTime);
    countdownGlow.Update(deltaTime);
    elapsed = std::min(totalDuration, elapsed + deltaTime);
    ApplyAnimation();
    if (elapsed < totalDuration)
        return false;

    active = false;
    countdown.setString("");
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

    if (countdown.getString().isEmpty())
        return;

    const sf::Color glowColor{
        countdown.getString().toAnsiString() == "GO!" ? FightColor : Cyan };
    countdownGlow.DrawBloom(
        target,
        countdown.getGlobalBounds(),
        [this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
        {
            glowTarget.draw(countdown, states);
        },
        glowColor);
    target.draw(countdown);
    countdownGlow.DrawHighlight(target, countdown.getGlobalBounds(), glowColor);
}

bool WaveIntro::IsActive() const noexcept
{
    return active;
}

void WaveIntro::ApplyAnimation()
{
    const float slideProgress{ SmoothStep(elapsed / SlideDuration) };
    const float titleY{ -90.f + (TitleTargetY + 90.f) * slideProgress };
    title.setFillColor(sf::Color(215, 247, 252));
    CenterText(title, { 960.f, titleY });

    const float countdownElapsed{ elapsed - SlideDuration - TitleHoldDuration };
    if (countdownElapsed < 0.f)
    {
        countdown.setString("");
        return;
    }
	if (!countdownSoundStarted)
	{
		countdownSoundStarted = true;
		audio.PlaySound(
			Config::Sound::Countdown,
			SoundGroup::Gameplay,
			100.f,
			1.f,
			SoundPlayback::Restart);
	}

    const std::size_t step{ std::min(
        static_cast<std::size_t>(countdownElapsed / countdownStepDuration),
        CountdownStepCount - 1u) };
    const float stepProgress{
        std::fmod(std::max(0.f, countdownElapsed), countdownStepDuration) /
		countdownStepDuration };
    const std::string& label{ Labels[step] };
    if (countdown.getString().toAnsiString() != label)
    {
        countdown.setString(label);
        countdownGlow.Invalidate();
    }

    const float opacityCurve{ std::sin(stepProgress * 3.14159265f) };
    const float scale{ 0.72f + 0.38f * SmoothStep(stepProgress) };
    const auto alpha{ static_cast<std::uint8_t>(
        std::clamp(opacityCurve * 1.35f, 0.f, 1.f) * 255.f) };
    const sf::Color baseColor{ label == "GO!" ? FightColor : sf::Color(215, 247, 252) };
    countdown.setFillColor(sf::Color(baseColor.r, baseColor.g, baseColor.b, alpha));
    countdown.setScale({ scale, scale });
    CenterText(countdown, { 960.f, CountdownY });
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
