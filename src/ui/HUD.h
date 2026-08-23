#pragma once

#include <cstddef>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include "ui/NeonGlow.h"

class Assets;
class GameplaySession;
class LocalizationManager;

namespace sf
{
	class RenderTarget;
}

class HUD
{
public:
	HUD(Assets& assets, const GameplaySession& session, LocalizationManager& localization);

	void Update(float deltaTime);
	void Draw(sf::RenderTarget& target);
	void HighlightScore(float duration) noexcept;
	void HighlightHealth(float duration) noexcept;
	void HighlightShield(float duration) noexcept;
	void HighlightParts(float duration) noexcept;
	void SetRunMode(bool enabled) noexcept;
	void SetPartsVisible(bool visible) noexcept;
	void SetScoreVisible(bool visible) noexcept;
	void SetSurvivalTime(float seconds) noexcept;

private:
	void UpdateScore(float deltaTime);
	void UpdateParts(float deltaTime);
	void UpdateHealthBar(float deltaTime);
	void UpdateShieldBar(float deltaTime);
	void UpdateHomingBar(float deltaTime);
	void UpdateWeaponBar(float deltaTime);
	void UpdateTimeSlowdownBar(float deltaTime);
	void UpdateBonusBarLayout();
	void CenterHealthText();
	void CenterShieldText();
	void CenterHomingText();
	void CenterWeaponText();
	void CenterTimeSlowdownText();
	void CenterScoreText();
	void CenterPartsText();
	void DrawScorePanel(sf::RenderTarget& target, const sf::RenderStates& states) const;
	void DrawPartsPanel(sf::RenderTarget& target, const sf::RenderStates& states) const;
	void RefreshLocalizedFonts();

	Assets& assets;
	const GameplaySession& session;
	LocalizationManager& localization;
	sf::Text scoreText;
	sf::Sprite scorePanel;
	NeonGlow scoreGlow;
	sf::Text partsText;
	sf::Sprite partsPanel;
	sf::Sprite partsIcon;
	NeonGlow partsGlow;
	sf::Text healthText;
	sf::Sprite healthFrame;
	sf::Sprite healthFill;
	NeonGlow healthGlow;
	sf::Text shieldText;
	sf::Sprite shieldFrame;
	sf::Sprite shieldFill;
	NeonGlow shieldGlow;
	sf::Text homingText;
	sf::Sprite homingFrame;
	sf::Sprite homingFill;
	NeonGlow homingGlow;
	sf::Text weaponText;
	sf::Sprite weaponFrame;
	sf::Sprite weaponFill;
	NeonGlow weaponGlow;
	sf::Text timeSlowdownText;
	sf::Sprite timeSlowdownFrame;
	sf::Sprite timeSlowdownFill;
	NeonGlow timeSlowdownGlow;
	std::size_t localizationRevision{ 0u };
	int displayedScore{ 0 };
	int displayedParts{ 0 };
	int displayedTimeSeconds{ -1 };
	float survivalSeconds{ 0.f };
	float scorePulseRemaining{ 0.f };
	float partsPulseRemaining{ 0.f };
	float tutorialScoreHighlightRemaining{ 0.f };
	float tutorialHealthHighlightRemaining{ 0.f };
	float tutorialShieldHighlightRemaining{ 0.f };
	float tutorialPartsHighlightRemaining{ 0.f };
	float criticalWarningRemaining{ 0.f };
	float blinkTimer{ 0.f };
	bool criticalWarningArmed{ true };
	float shieldBlinkTimer{ 0.f };
	bool shieldVisible{ false };
	bool homingVisible{ false };
	bool weaponVisible{ false };
	bool timeSlowdownVisible{ false };
	bool runMode{ false };
	bool partsVisible{ true };
	bool scoreVisible{ true };

	static constexpr float CriticalThreshold{ 0.3f };
	static constexpr float CriticalWarningDuration{ 3.f };
	static constexpr float BlinkInterval{ 0.16f };
	static constexpr float ScorePulseDuration{ 0.38f };
	static constexpr float PartsPulseDuration{ 0.72f };
};
