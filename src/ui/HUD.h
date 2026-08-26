#pragma once

#include <cstddef>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "rendering/NeonGlow.h"
#include "ui/LocalizationRevision.h"

class Assets;
class GameplaySession;
class LocalizationManager;

namespace sf
{
	class RenderTarget;
}

namespace UI
{
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

		void SetRunMode(bool needToEnableRunMode) noexcept;
		void SetPartsVisible(bool needToShowParts) noexcept;
		void SetScoreVisible(bool needToShowScore) noexcept;
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

		// Shared by all five resource bars (health/shield/homing/weapon/timeSlowdown):
		// draws frame, then (if the fill isn't empty) a quantized glow behind a
		// white-tinted copy of the fill, then the fill itself, then the text.
		void DrawResourceBar(sf::RenderTarget& target, NeonGlow& glowEffect,
			const sf::Sprite& frame, const sf::Sprite& fill, const sf::Text& text) const;

		// The pulsing bloom shown while a tutorial highlight is active on a
		// resource bar (health/shield only -- the other three bars have no
		// tutorial highlight). No-op if remainingSeconds <= 0.
		void DrawResourceBarTutorialHighlight(sf::RenderTarget& target, NeonGlow& glowEffect,
			const sf::Sprite& frame, const sf::Sprite& fill, const sf::Text& text,
			float remainingSeconds, sf::Color highlightColor) const;

		void RefreshLocalizedFonts();

		Assets& assets;
		const GameplaySession& session;
		LocalizationManager& localization;

		sf::Text scoreText;
		sf::Sprite scorePanel;
		NeonGlow scoreGlowEffect;

		sf::Text partsText;
		sf::Sprite partsPanel;
		sf::Sprite partsIcon;
		NeonGlow partsGlowEffect;

		sf::Text healthText;
		sf::Sprite healthFrame;
		sf::Sprite healthFill;
		NeonGlow healthGlowEffect;

		sf::Text shieldText;
		sf::Sprite shieldFrame;
		sf::Sprite shieldFill;
		NeonGlow shieldGlowEffect;

		sf::Text homingText;
		sf::Sprite homingFrame;
		sf::Sprite homingFill;
		NeonGlow homingGlowEffect;

		sf::Text weaponText;
		sf::Sprite weaponFrame;
		sf::Sprite weaponFill;
		NeonGlow weaponGlowEffect;

		sf::Text timeSlowdownText;
		sf::Sprite timeSlowdownFrame;
		sf::Sprite timeSlowdownFill;
		NeonGlow timeSlowdownGlowEffect;

		LocalizationRevision localizationRevision;

		int displayedScore = 0;
		int displayedParts = 0;
		int displayedTimeSeconds = -1;
		float survivalSeconds = 0.f;
		float scorePulseRemaining = 0.f;
		float partsPulseRemaining = 0.f;
		float tutorialScoreHighlightRemaining = 0.f;
		float tutorialHealthHighlightRemaining = 0.f;
		float tutorialShieldHighlightRemaining = 0.f;
		float tutorialPartsHighlightRemaining = 0.f;
		float criticalWarningRemaining = 0.f;
		float blinkTimer = 0.f;
		bool isCriticalWarningArmed = true;
		float shieldBlinkTimer = 0.f;

		bool isShieldVisible = false;
		bool isHomingVisible = false;
		bool isWeaponVisible = false;
		bool isTimeSlowdownVisible = false;

		bool isRunMode = false;
		bool isPartsVisible = true;
		bool isScoreVisible = true;

		static constexpr float CriticalThreshold = 0.3f;
		static constexpr float CriticalWarningDuration = 3.f;
		static constexpr float BlinkInterval = 0.16f;
		static constexpr float ScorePulseDuration = 0.38f;
		static constexpr float PartsPulseDuration = 0.72f;
	};
}