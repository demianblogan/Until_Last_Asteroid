#pragma once
#include <optional>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include "achievements/AchievementManager.h"
#include "ui/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class Assets;
class AudioManager;
class LocalizationManager;
namespace sf { class RenderTarget; }

class AchievementToast
{
public:
	AchievementToast(Assets& assets, AudioManager& audio,
		AchievementManager& achievements, LocalizationManager& localization,
		sf::Vector2f logicalSize);
	void Update(float deltaTime);
	void Draw(sf::RenderTarget& target);
private:
	void Begin(AchievementID id);
	void Layout(float y);
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
	NeonGlow glow;
	sf::Vector2f logicalSize;
	std::size_t localizationRevision{ 0u };
	float elapsed{ 0.f };
	bool active{ false };
};
