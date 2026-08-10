#include "HUD.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <SFML/Graphics/RenderTarget.hpp>
#include "assets/AssetStore.h"
#include "game/GameplaySession.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr float HealthBarScale{ 0.5f };
	constexpr sf::Vector2f FramePosition{ 20.f, 1010.f };
	constexpr sf::Vector2f FillOffset{ 27.f * HealthBarScale, 19.f * HealthBarScale };
	constexpr sf::Vector2f FrameSize{ 640.f * HealthBarScale, 100.f * HealthBarScale };

	sf::Color LerpColor(const sf::Color& from, const sf::Color& to, float amount)
	{
		amount = std::clamp(amount, 0.f, 1.f);
		return sf::Color(
			static_cast<std::uint8_t>(std::lerp(from.r, to.r, amount)),
			static_cast<std::uint8_t>(std::lerp(from.g, to.g, amount)),
			static_cast<std::uint8_t>(std::lerp(from.b, to.b, amount)));
	}
}

HUD::HUD(AssetStore& assets, const GameplaySession& session)
	: session(session)
	, scoreText(assets.Fonts().Get(Config::Font::GUI))
	, healthText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, healthFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, healthFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, healthGlow(assets)
{
	scoreText.setCharacterSize(50);
	scoreText.setPosition({ 20.f, 20.f });

	healthFrame.setPosition(FramePosition);
	healthFrame.setScale({ HealthBarScale, HealthBarScale });
	healthFill.setPosition(FramePosition + FillOffset);
	healthFill.setScale({ HealthBarScale, HealthBarScale });
	healthText.setCharacterSize(18);
	healthText.setFillColor(sf::Color::White);
	healthText.setOutlineColor(sf::Color(0, 10, 20, 210));
	healthText.setOutlineThickness(2.f);
	Update(0.f);
}

void HUD::Update(float deltaTime)
{
	scoreText.setString(std::to_string(session.GetScore()));
	UpdateHealthBar(deltaTime);
}

void HUD::UpdateHealthBar(float deltaTime)
{
	const Health& health{ session.GetPlayerHealth() };
	const float ratio{ std::clamp(health.GetRatio(), 0.f, 1.f) };
	const sf::Vector2u textureSize{ healthFill.getTexture().getSize() };
	const int visibleWidth{ static_cast<int>(std::round(textureSize.x * ratio)) };
	healthFill.setTextureRect(sf::IntRect(
		{ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));

	const sf::Color red{ 255, 55, 48 };
	const sf::Color yellow{ 255, 215, 45 };
	const sf::Color green{ 55, 235, 105 };
	sf::Color fillColor{ ratio >= 0.5f
		? LerpColor(yellow, green, (ratio - 0.5f) * 2.f)
		: LerpColor(red, yellow, ratio * 2.f) };

	if (ratio > CriticalThreshold)
	{
		criticalWarningArmed = true;
		criticalWarningRemaining = 0.f;
		blinkTimer = 0.f;
	}
	else if (criticalWarningArmed && health.GetCurrent() > 0)
	{
		criticalWarningArmed = false;
		criticalWarningRemaining = CriticalWarningDuration;
		blinkTimer = 0.f;
	}

	if (criticalWarningRemaining > 0.f)
	{
		criticalWarningRemaining = std::max(0.f, criticalWarningRemaining - deltaTime);
		blinkTimer += deltaTime;
		if (static_cast<int>(blinkTimer / BlinkInterval) % 2 != 0)
			fillColor.a = 45;
	}
	healthFill.setColor(fillColor);

	const int percentage{ static_cast<int>(std::round(ratio * 100.f)) };
	healthText.setString(std::to_string(percentage) + "%");
	CenterHealthText();
}

void HUD::CenterHealthText()
{
	const sf::FloatRect bounds{ healthText.getLocalBounds() };
	healthText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f });
	healthText.setPosition(FramePosition + FrameSize * 0.5f);
}

void HUD::Draw(sf::RenderTarget& target)
{
	target.draw(scoreText);
	target.draw(healthFrame);
	if (healthFill.getTextureRect().size.x > 0)
	{
		healthGlow.DrawBloom(
			target,
			healthFill.getGlobalBounds(),
			[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				sf::Sprite glowSource{ healthFill };
				glowSource.setColor(sf::Color::White);
				glowTarget.draw(glowSource, states);
			},
			healthFill.getColor(),
			false);
	}
	// The frame texture also contains the opaque dark backing. Draw the fill on
	// top of it so that the backing cannot hide the changing health amount.
	target.draw(healthFill);
	target.draw(healthText);
}
