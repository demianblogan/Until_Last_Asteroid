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

private:
	void UpdateHealthBar(float deltaTime);
	void CenterHealthText();

	const GameplaySession& session;
	sf::Text scoreText;
	sf::Text healthText;
	sf::Sprite healthFrame;
	sf::Sprite healthFill;
	NeonGlow healthGlow;
	float criticalWarningRemaining{ 0.f };
	float blinkTimer{ 0.f };
	bool criticalWarningArmed{ true };

	static constexpr float CriticalThreshold{ 0.3f };
	static constexpr float CriticalWarningDuration{ 3.f };
	static constexpr float BlinkInterval{ 0.16f };
};
