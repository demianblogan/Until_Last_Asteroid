#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include "ui/NeonGlow.h"

class AssetStore;
class GameplaySession;

namespace sf
{
	class RenderTarget;
}

class HUD
{
public:
	HUD(AssetStore& assets, const GameplaySession& session);

	void Update(float deltaTime);
	void Draw(sf::RenderTarget& target);
	void HighlightScore(float duration) noexcept;
	void HighlightHealth(float duration) noexcept;
	void HighlightShield(float duration) noexcept;

private:
	void UpdateScore(float deltaTime);
	void UpdateHealthBar(float deltaTime);
	void UpdateShieldBar(float deltaTime);
	void CenterHealthText();
	void CenterShieldText();
	void CenterScoreText();
	void DrawScorePanel(sf::RenderTarget& target, const sf::RenderStates& states) const;

	const GameplaySession& session;
	sf::Text scoreText;
	sf::Sprite scorePanel;
	NeonGlow scoreGlow;
	sf::Text healthText;
	sf::Sprite healthFrame;
	sf::Sprite healthFill;
	NeonGlow healthGlow;
	sf::Text shieldText;
	sf::Sprite shieldFrame;
	sf::Sprite shieldFill;
	NeonGlow shieldGlow;
	int displayedScore{ 0 };
	float scorePulseRemaining{ 0.f };
	float tutorialScoreHighlightRemaining{ 0.f };
	float tutorialHealthHighlightRemaining{ 0.f };
	float tutorialShieldHighlightRemaining{ 0.f };
	float criticalWarningRemaining{ 0.f };
	float blinkTimer{ 0.f };
	bool criticalWarningArmed{ true };
	float shieldBlinkTimer{ 0.f };
	bool shieldVisible{ false };

	static constexpr float CriticalThreshold{ 0.3f };
	static constexpr float CriticalWarningDuration{ 3.f };
	static constexpr float BlinkInterval{ 0.16f };
	static constexpr float ScorePulseDuration{ 0.38f };
};
