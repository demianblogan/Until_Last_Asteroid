#include "WaveIntro.h"

#include <algorithm>
#include <cstdint>
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/Assets.h"
#include "localization/LocalizationManager.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace UI
{
	namespace
	{
		constexpr float SlideDuration = 0.45f;
		constexpr float TitleHoldDuration = 0.55f;
		constexpr float FadeDuration = 0.30f;
		constexpr float TotalDuration = SlideDuration + TitleHoldDuration + FadeDuration;
		constexpr float TitleTargetY = 540.f;
		constexpr sf::Color Cyan{ 25, 220, 255 };

		float SmoothStep(float value)
		{
			value = std::clamp(value, 0.f, 1.f);
			return value * value * (3.f - 2.f * value);
		}
	}

	WaveIntro::WaveIntro(Assets& assets, LocalizationManager& localize)
		: titleGlow(assets), assets(assets), localization(localize)
		, title(assets.Fonts().Get(localize.GetBoldFont()), "", 76)
	{
		title.setOutlineColor(sf::Color(2, 12, 24, 235));
		title.setOutlineThickness(4.f);
		title.setLetterSpacing(1.1f);
	}

	void WaveIntro::Start(int waveNumber, bool isFinalWave)
	{
		title.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		title.setString(isFinalWave ? localization.GetText("intro.final_wave")
			: localization.FormatText("intro.wave", "value", std::to_string(std::max(1, waveNumber))));

		elapsedSeconds = 0.f;
		isActive = true;
		titleGlow.Invalidate();

		ApplyAnimation();
	}

	bool WaveIntro::Update(float deltaTime)
	{
		if (!isActive)
			return false;

		titleGlow.Update(deltaTime);
		elapsedSeconds = std::min(TotalDuration, elapsedSeconds + deltaTime);

		ApplyAnimation();

		if (elapsedSeconds < TotalDuration)
			return false;

		isActive = false;
		return true;
	}

	void WaveIntro::Draw(sf::RenderTarget& target)
	{
		if (!isActive)
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

	void WaveIntro::Reset() noexcept
	{
		isActive = false;
		elapsedSeconds = 0.f;
	}

	bool WaveIntro::IsActive() const noexcept
	{
		return isActive;
	}

	void WaveIntro::ApplyAnimation()
	{
		const float slideProgress = SmoothStep(elapsedSeconds / SlideDuration);
		const float titleY = -90.f + (TitleTargetY + 90.f) * slideProgress;
		const float fadeProgress = elapsedSeconds <= SlideDuration + TitleHoldDuration
			? 0.f
			: SmoothStep((elapsedSeconds - SlideDuration - TitleHoldDuration) / FadeDuration);
		const auto alpha = static_cast<std::uint8_t>((1.f - fadeProgress) * 255.f);

		title.setFillColor(sf::Color(215, 247, 252, alpha));
		title.setOutlineColor(sf::Color(2, 12, 24, alpha));
		title.setScale({ 1.f + fadeProgress * 0.06f, 1.f + fadeProgress * 0.06f });

		TextLayout::CenterText(title, { 960.f, titleY });
	}
}