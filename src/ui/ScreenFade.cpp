#include "ScreenFade.h"

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/RenderTarget.hpp>

namespace UI
{
	ScreenFade::ScreenFade(sf::Vector2f size)
		: overlay(size)
	{
		overlay.setFillColor(sf::Color::Transparent);
	}

	void ScreenFade::StartFadeIn(float fadeDuration)
	{
		Start(Direction::In, fadeDuration);
	}

	void ScreenFade::StartFadeOut(float fadeDuration)
	{
		Start(Direction::Out, fadeDuration);
	}

	void ScreenFade::Start(Direction fadeDirection, float fadeDuration)
	{
		direction = fadeDirection;
		fadeDurationSeconds = std::max(0.001f, fadeDuration);
		elapsedSeconds = 0.f;
		isActive = true;

		ApplyOpacity(0.f);
	}

	void ScreenFade::Update(float deltaTime)
	{
		if (!isActive)
			return;

		elapsedSeconds = std::min(fadeDurationSeconds, elapsedSeconds + deltaTime);
		ApplyOpacity(elapsedSeconds / fadeDurationSeconds);
		isActive = elapsedSeconds < fadeDurationSeconds;
	}

	void ScreenFade::ApplyOpacity(float progress)
	{
		progress = std::clamp(progress, 0.f, 1.f);
		const float eased = progress * progress * (3.f - 2.f * progress);
		const float opacity = direction == Direction::Out ? eased : 1.f - eased;
		overlay.setFillColor(sf::Color(0, 0, 0, static_cast<std::uint8_t>(opacity * 255.f)));
	}

	void ScreenFade::Draw(sf::RenderTarget& target) const
	{
		if (overlay.getFillColor().a > 0u)
			target.draw(overlay);
	}

	bool ScreenFade::IsActive() const noexcept
	{
		return isActive;
	}
}