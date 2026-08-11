#include "HUD.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include "assets/AssetStore.h"
#include "game/GameplaySession.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr float HealthBarScale{ 0.5f };
	constexpr float ScorePanelScale{ 0.5f };
	constexpr sf::Vector2f ScorePanelPosition{ 20.f, 20.f };
	constexpr sf::Vector2f ScorePanelSize{ 387.f, 74.f };
	constexpr sf::Vector2f FramePosition{ 20.f, 1010.f };
	constexpr sf::Vector2f ShieldFramePosition{ 20.f, 950.f };
	constexpr sf::Vector2f FillOffset{ 27.f * HealthBarScale, 19.f * HealthBarScale };
	constexpr sf::Vector2f FrameSize{ 640.f * HealthBarScale, 100.f * HealthBarScale };

	sf::Color LerpColor(const sf::Color& from, const sf::Color& to, float amount)
	{
		amount = std::clamp(amount, 0.f, 1.f);
		return sf::Color(
			static_cast<std::uint8_t>(std::lerp(from.r, to.r, amount)),
			static_cast<std::uint8_t>(std::lerp(from.g, to.g, amount)),
			static_cast<std::uint8_t>(std::lerp(from.b, to.b, amount)),
			static_cast<std::uint8_t>(std::lerp(from.a, to.a, amount)));
	}
}

HUD::HUD(AssetStore& assets, const GameplaySession& session)
	: session(session)
	, scoreText(assets.Fonts().Get(Config::Font::MenuRegular))
	, scorePanel(assets.Textures().Get(Config::Texture::ScorePanelFrame))
	, scoreGlow(assets)
	, healthText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, healthFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, healthFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, healthGlow(assets)
	, shieldText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, shieldFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, shieldFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, shieldGlow(assets)
{
	scorePanel.setPosition(ScorePanelPosition);
	scorePanel.setScale({ ScorePanelScale, ScorePanelScale });

	scoreText.setCharacterSize(28);
	scoreText.setLetterSpacing(1.04f);
	scoreText.setFillColor(sf::Color(226, 249, 255));
	scoreText.setOutlineColor(sf::Color(4, 24, 38, 230));
	scoreText.setOutlineThickness(2.f);
	displayedScore = session.GetScore();
	scoreText.setString("Score: " + std::to_string(displayedScore));
	CenterScoreText();

	healthFrame.setPosition(FramePosition);
	healthFrame.setScale({ HealthBarScale, HealthBarScale });
	healthFill.setPosition(FramePosition + FillOffset);
	healthFill.setScale({ HealthBarScale, HealthBarScale });
	healthText.setCharacterSize(18);
	healthText.setFillColor(sf::Color::White);
	healthText.setOutlineColor(sf::Color(0, 10, 20, 210));
	healthText.setOutlineThickness(2.f);
	shieldFrame.setPosition(ShieldFramePosition);
	shieldFrame.setScale({ HealthBarScale, HealthBarScale });
	shieldFill.setPosition(ShieldFramePosition + FillOffset);
	shieldFill.setScale({ HealthBarScale, HealthBarScale });
	shieldFill.setColor(sf::Color(35, 225, 245));
	shieldText.setCharacterSize(18);
	shieldText.setFillColor(sf::Color(215, 255, 255));
	shieldText.setOutlineColor(sf::Color(0, 10, 20, 210));
	shieldText.setOutlineThickness(2.f);
	Update(0.f);
}

void HUD::Update(float deltaTime)
{
	tutorialScoreHighlightRemaining = std::max(
		0.f, tutorialScoreHighlightRemaining - deltaTime);
	tutorialHealthHighlightRemaining = std::max(
		0.f, tutorialHealthHighlightRemaining - deltaTime);
	tutorialShieldHighlightRemaining = std::max(
		0.f, tutorialShieldHighlightRemaining - deltaTime);
	UpdateScore(deltaTime);
	UpdateHealthBar(deltaTime);
	UpdateShieldBar(deltaTime);
}

void HUD::HighlightScore(float duration) noexcept
{
	tutorialScoreHighlightRemaining = std::max(tutorialScoreHighlightRemaining, duration);
	scoreGlow.Invalidate();
}

void HUD::HighlightHealth(float duration) noexcept
{
	tutorialHealthHighlightRemaining = std::max(tutorialHealthHighlightRemaining, duration);
	healthGlow.Invalidate();
}

void HUD::HighlightShield(float duration) noexcept
{
	tutorialShieldHighlightRemaining = std::max(tutorialShieldHighlightRemaining, duration);
	shieldGlow.Invalidate();
}

void HUD::UpdateShieldBar(float deltaTime)
{
	const Shield& shield{ session.GetPlayerShield() };
	shieldVisible = shield.IsActive();
	if (!shieldVisible)
	{
		shieldFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
		shieldBlinkTimer = 0.f;
		return;
	}

	shieldGlow.Update(deltaTime);
	const float ratio{ std::clamp(shield.GetRatio(), 0.f, 1.f) };
	const sf::Vector2u textureSize{ shieldFill.getTexture().getSize() };
	const int visibleWidth{ static_cast<int>(std::round(textureSize.x * ratio)) };
	shieldFill.setTextureRect(sf::IntRect(
		{ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));

	sf::Color color{ 35, 225, 245 };
	if (ratio <= 0.25f)
	{
		shieldBlinkTimer += deltaTime;
		if (static_cast<int>(shieldBlinkTimer / 0.1f) % 2 != 0)
			color.a = 45;
	}
	else
	{
		shieldBlinkTimer = 0.f;
	}
	shieldFill.setColor(color);

	const int percentage{ static_cast<int>(std::ceil(ratio * 100.f)) };
	shieldText.setString(std::to_string(percentage) + "%");
	CenterShieldText();
}

void HUD::UpdateScore(float deltaTime)
{
	scoreGlow.Update(deltaTime);
	const int currentScore{ session.GetScore() };
	if (currentScore != displayedScore)
	{
		if (currentScore > displayedScore)
			scorePulseRemaining = ScorePulseDuration;

		displayedScore = currentScore;
		scoreText.setString("Score: " + std::to_string(displayedScore));
		CenterScoreText();
		scoreGlow.Invalidate();
	}

	scorePulseRemaining = std::max(0.f, scorePulseRemaining - deltaTime);
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

void HUD::CenterShieldText()
{
	const sf::FloatRect bounds{ shieldText.getLocalBounds() };
	shieldText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f });
	shieldText.setPosition(ShieldFramePosition + FrameSize * 0.5f);
}

void HUD::CenterScoreText()
{
	const sf::FloatRect bounds{ scoreText.getLocalBounds() };
	scoreText.setOrigin({
		bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f
	});
	scoreText.setPosition(ScorePanelPosition + ScorePanelSize * 0.5f);
}

void HUD::DrawScorePanel(sf::RenderTarget& target, const sf::RenderStates& states) const
{
	target.draw(scorePanel, states);
	target.draw(scoreText, states);
}

void HUD::Draw(sf::RenderTarget& target)
{
	if (scorePulseRemaining > 0.f || tutorialScoreHighlightRemaining > 0.f)
	{
		const float normalized{ scorePulseRemaining / ScorePulseDuration };
		const float tutorialFlash{ tutorialScoreHighlightRemaining > 0.f
			? 0.35f + 0.65f * std::abs(std::sin(tutorialScoreHighlightRemaining * 9.f))
			: 0.f };
		const float flash{ std::max(normalized * normalized, tutorialFlash) };
		const sf::Color flashColor{ LerpColor(
			sf::Color::Black,
			sf::Color(170, 250, 255),
			flash) };
		scoreGlow.DrawBloom(
			target,
			scorePanel.getGlobalBounds(),
			[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				DrawScorePanel(glowTarget, states);
			},
			flashColor,
			false);
	}
	DrawScorePanel(target, sf::RenderStates::Default);
	if (scorePulseRemaining > 0.f || tutorialScoreHighlightRemaining > 0.f)
	{
		const float normalized{ scorePulseRemaining / ScorePulseDuration };
		const float tutorialFlash{ tutorialScoreHighlightRemaining > 0.f
			? 0.35f + 0.65f * std::abs(std::sin(tutorialScoreHighlightRemaining * 9.f))
			: 0.f };
		const float flash{ std::max(normalized * normalized, tutorialFlash) };
		scoreGlow.DrawHighlight(
			target,
			scorePanel.getGlobalBounds(),
			LerpColor(sf::Color::Black, sf::Color(205, 255, 255), flash));
	}
	if (tutorialHealthHighlightRemaining > 0.f)
	{
		const float flash{ 0.35f +
			0.65f * std::abs(std::sin(tutorialHealthHighlightRemaining * 9.f)) };
		healthGlow.DrawBloom(
			target,
			healthFrame.getGlobalBounds(),
			[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				glowTarget.draw(healthFrame, states);
				glowTarget.draw(healthFill, states);
				glowTarget.draw(healthText, states);
			},
			LerpColor(sf::Color::Black, sf::Color(100, 255, 170), flash),
			false);
	}
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

	if (shieldVisible)
	{
		if (tutorialShieldHighlightRemaining > 0.f)
		{
			const float flash{ 0.35f +
				0.65f * std::abs(std::sin(tutorialShieldHighlightRemaining * 9.f)) };
			shieldGlow.DrawBloom(
				target,
				shieldFrame.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					glowTarget.draw(shieldFrame, states);
					glowTarget.draw(shieldFill, states);
					glowTarget.draw(shieldText, states);
				},
				LerpColor(sf::Color::Black, sf::Color(80, 245, 255), flash),
				false);
		}
		target.draw(shieldFrame);
		if (shieldFill.getTextureRect().size.x > 0)
		{
			shieldGlow.DrawBloom(
				target,
				shieldFill.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					sf::Sprite glowSource{ shieldFill };
					glowSource.setColor(sf::Color::White);
					glowTarget.draw(glowSource, states);
				},
				shieldFill.getColor(),
				false);
		}
		target.draw(shieldFill);
		target.draw(shieldText);
	}
}
