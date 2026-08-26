#pragma once

#include <optional>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "achievements/AchievementManager.h"
#include "rendering/NeonGlow.h"
#include "ui/LocalizationRevision.h"
#include "ui/RoundedRectangleShape.h"

class Assets;
class AudioManager;
class LocalizationManager;

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	class AchievementToast
	{
	public:
		AchievementToast(Assets& assets, AudioManager& audio,
			AchievementManager& achievements, LocalizationManager& localization,
			sf::Vector2f logicalSize);

		void Update(float deltaTime);
		void Draw(sf::RenderTarget& target);

	private:
		void BeginToast(AchievementID id);
		void PositionElements(float y);
		void RefreshLocalizedContent();

		Assets& assets;
		AudioManager& audio;
		AchievementManager& achievements;
		LocalizationManager& localization;

		RoundedRectangleShape panel;
		std::optional<sf::Sprite> icon;
		sf::Text unlockedLabel;
		sf::Text title;
		sf::Text description;
		Rendering::NeonGlow glowEffect;

		sf::Vector2f logicalSize;
		LocalizationRevision localizationRevision;
		float toastElapsedSeconds = 0.f;
		bool isShowingToast = false;
	};
}