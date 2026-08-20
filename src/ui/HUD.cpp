#include "HUD.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>
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
	constexpr sf::Vector2f PartsPanelPosition{ 1513.f, 986.f };
	constexpr sf::Vector2f ScorePanelSize{ 387.f, 74.f };
	constexpr sf::Vector2f FramePosition{ 20.f, 1010.f };
	constexpr float BonusBarSpacing{ 60.f };
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
	, partsText(assets.Fonts().Get(Config::Font::MenuRegular))
	, partsPanel(assets.Textures().Get(Config::Texture::ScorePanelFrame))
	, partsIcon(assets.Textures().Get(Config::Texture::PartToken))
	, partsGlow(assets)
	, healthText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, healthFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, healthFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, healthGlow(assets)
	, shieldText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, shieldFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, shieldFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, shieldGlow(assets)
	, homingText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, homingFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, homingFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, homingGlow(assets)
	, weaponText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, weaponFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, weaponFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, weaponGlow(assets)
	, timeSlowdownText(assets.Fonts().Get(Config::Font::MenuSemibold))
	, timeSlowdownFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
	, timeSlowdownFill(assets.Textures().Get(Config::Texture::HealthBarFill))
	, timeSlowdownGlow(assets)
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

	partsPanel.setPosition(PartsPanelPosition);
	partsPanel.setScale({ ScorePanelScale, ScorePanelScale });
	const sf::Vector2u partTextureSize{ partsIcon.getTexture().getSize() };
	const float partIconScale{ 48.f / static_cast<float>(
		std::max(partTextureSize.x, partTextureSize.y)) };
	partsIcon.setScale({ partIconScale, partIconScale });
	partsIcon.setOrigin({
		static_cast<float>(partTextureSize.x) * 0.5f,
		static_cast<float>(partTextureSize.y) * 0.5f });
	partsIcon.setPosition(PartsPanelPosition + sf::Vector2f{ 55.f, 37.f });
	partsText.setCharacterSize(26);
	partsText.setLetterSpacing(1.04f);
	partsText.setFillColor(sf::Color(255, 224, 145));
	partsText.setOutlineColor(sf::Color(4, 24, 38, 230));
	partsText.setOutlineThickness(2.f);
	displayedParts = session.GetDisplayedParts();
	partsText.setString("PARTS: " + std::to_string(displayedParts));
	CenterPartsText();

	healthFrame.setPosition(FramePosition);
	healthFrame.setScale({ HealthBarScale, HealthBarScale });
	healthFill.setPosition(FramePosition + FillOffset);
	healthFill.setScale({ HealthBarScale, HealthBarScale });
	healthText.setCharacterSize(18);
	healthText.setFillColor(sf::Color::White);
	healthText.setOutlineColor(sf::Color(0, 10, 20, 210));
	healthText.setOutlineThickness(2.f);
	shieldFrame.setPosition({ FramePosition.x, FramePosition.y - BonusBarSpacing });
	shieldFrame.setScale({ HealthBarScale, HealthBarScale });
	shieldFill.setPosition(shieldFrame.getPosition() + FillOffset);
	shieldFill.setScale({ HealthBarScale, HealthBarScale });
	shieldFill.setColor(sf::Color(35, 225, 245));
	shieldText.setCharacterSize(18);
	shieldText.setFillColor(sf::Color(215, 255, 255));
	shieldText.setOutlineColor(sf::Color(0, 10, 20, 210));
	shieldText.setOutlineThickness(2.f);
	homingFrame.setPosition({ FramePosition.x, FramePosition.y - BonusBarSpacing * 2.f });
	homingFrame.setScale({ HealthBarScale, HealthBarScale });
	homingFill.setPosition(homingFrame.getPosition() + FillOffset);
	homingFill.setScale({ HealthBarScale, HealthBarScale });
	homingFill.setColor(sf::Color(255, 190, 40));
	homingText.setCharacterSize(18);
	homingText.setFillColor(sf::Color(255, 239, 185));
	homingText.setOutlineColor(sf::Color(20, 12, 0, 220));
	homingText.setOutlineThickness(2.f);
	weaponFrame.setPosition({ FramePosition.x, FramePosition.y - BonusBarSpacing * 3.f });
	weaponFrame.setScale({ HealthBarScale, HealthBarScale });
	weaponFill.setPosition(weaponFrame.getPosition() + FillOffset);
	weaponFill.setScale({ HealthBarScale, HealthBarScale });
	weaponText.setCharacterSize(18);
	weaponText.setOutlineColor(sf::Color(18, 4, 2, 220));
	weaponText.setOutlineThickness(2.f);
	timeSlowdownFrame.setPosition({ FramePosition.x, FramePosition.y - BonusBarSpacing * 3.f });
	timeSlowdownFrame.setScale({ HealthBarScale, HealthBarScale });
	timeSlowdownFill.setPosition(timeSlowdownFrame.getPosition() + FillOffset);
	timeSlowdownFill.setScale({ HealthBarScale, HealthBarScale });
	timeSlowdownFill.setColor(sf::Color(180, 75, 255));
	timeSlowdownText.setCharacterSize(18);
	timeSlowdownText.setFillColor(sf::Color(238, 215, 255));
	timeSlowdownText.setOutlineColor(sf::Color(13, 2, 24, 220));
	timeSlowdownText.setOutlineThickness(2.f);
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
	tutorialPartsHighlightRemaining = std::max(
		0.f, tutorialPartsHighlightRemaining - deltaTime);
	UpdateScore(deltaTime);
	UpdateParts(deltaTime);
	UpdateHealthBar(deltaTime);
	UpdateShieldBar(deltaTime);
	UpdateHomingBar(deltaTime);
	UpdateWeaponBar(deltaTime);
	UpdateTimeSlowdownBar(deltaTime);
	UpdateBonusBarLayout();
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

void HUD::SetRunMode(bool enabled) noexcept
{
	runMode = enabled;
	displayedTimeSeconds = -1;
	UpdateScore(0.f);
}

void HUD::SetPartsVisible(bool visible) noexcept
{
	partsVisible = visible;
}

void HUD::SetSurvivalTime(float seconds) noexcept
{
	survivalSeconds = std::max(0.f, seconds);
}

void HUD::HighlightParts(float duration) noexcept
{
	tutorialPartsHighlightRemaining = std::max(tutorialPartsHighlightRemaining, duration);
	partsGlow.Invalidate();
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
	shieldText.setString("SHIELD " + std::to_string(percentage) + "%");
}

void HUD::UpdateHomingBar(float deltaTime)
{
	homingVisible = session.IsHomingBulletsActive();
	if (!homingVisible)
	{
		homingFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
		return;
	}

	homingGlow.Update(deltaTime);
	const float ratio{ std::clamp(session.GetHomingBulletsRatio(), 0.f, 1.f) };
	const sf::Vector2u textureSize{ homingFill.getTexture().getSize() };
	const int visibleWidth{ static_cast<int>(std::round(textureSize.x * ratio)) };
	homingFill.setTextureRect(sf::IntRect(
		{ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));
	homingFill.setColor(sf::Color(255, 190, 40));

	const int percentage{ static_cast<int>(std::ceil(ratio * 100.f)) };
	homingText.setString("HOMING " + std::to_string(percentage) + "%");
}

void HUD::UpdateWeaponBar(float deltaTime)
{
	using WeaponMode = GameplaySession::WeaponMode;
	const WeaponMode mode{ session.GetWeaponMode() };
	weaponVisible = mode != WeaponMode::Normal;
	if (!weaponVisible)
	{
		weaponFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
		return;
	}

	weaponGlow.Update(deltaTime);
	const float ratio{ std::clamp(session.GetWeaponBonusRatio(), 0.f, 1.f) };
	const sf::Vector2u textureSize{ weaponFill.getTexture().getSize() };
	const int visibleWidth{ static_cast<int>(std::round(textureSize.x * ratio)) };
	weaponFill.setTextureRect(sf::IntRect(
		{ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));
	const int percentage{ static_cast<int>(std::ceil(ratio * 100.f)) };
	if (mode == WeaponMode::Laser)
	{
		weaponFill.setColor(sf::Color(255, 55, 28));
		weaponText.setFillColor(sf::Color(255, 218, 200));
		weaponText.setString("LASER " + std::to_string(percentage) + "%");
	}
	else
	{
		weaponFill.setColor(sf::Color(255, 170, 30));
		weaponText.setFillColor(sf::Color(255, 238, 185));
		weaponText.setString("TRIPLE SHOT " + std::to_string(percentage) + "%");
	}
}

void HUD::UpdateTimeSlowdownBar(float deltaTime)
{
	timeSlowdownVisible = session.IsTimeSlowdownActive();
	if (!timeSlowdownVisible)
	{
		timeSlowdownFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
		return;
	}

	timeSlowdownGlow.Update(deltaTime);
	const float ratio{ std::clamp(session.GetTimeSlowdownRatio(), 0.f, 1.f) };
	const sf::Vector2u textureSize{ timeSlowdownFill.getTexture().getSize() };
	const int visibleWidth{ static_cast<int>(std::round(textureSize.x * ratio)) };
	timeSlowdownFill.setTextureRect(sf::IntRect(
		{ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));
	timeSlowdownFill.setColor(sf::Color(180, 75, 255));

	const int percentage{ static_cast<int>(std::ceil(ratio * 100.f)) };
	timeSlowdownText.setString("TIME SLOW " + std::to_string(percentage) + "%");
}

void HUD::UpdateBonusBarLayout()
{
	float nextY{ FramePosition.y - BonusBarSpacing };
	if (shieldVisible)
	{
		shieldFrame.setPosition({ FramePosition.x, nextY });
		shieldFill.setPosition(shieldFrame.getPosition() + FillOffset);
		CenterShieldText();
		nextY -= BonusBarSpacing;
	}
	if (homingVisible)
	{
		homingFrame.setPosition({ FramePosition.x, nextY });
		homingFill.setPosition(homingFrame.getPosition() + FillOffset);
		CenterHomingText();
		nextY -= BonusBarSpacing;
	}
	if (weaponVisible)
	{
		weaponFrame.setPosition({ FramePosition.x, nextY });
		weaponFill.setPosition(weaponFrame.getPosition() + FillOffset);
		CenterWeaponText();
		nextY -= BonusBarSpacing;
	}
	if (timeSlowdownVisible)
	{
		timeSlowdownFrame.setPosition({ FramePosition.x, nextY });
		timeSlowdownFill.setPosition(timeSlowdownFrame.getPosition() + FillOffset);
		CenterTimeSlowdownText();
	}
}

void HUD::UpdateScore(float deltaTime)
{
	scoreGlow.Update(deltaTime);
	if (runMode)
	{
		const int totalSeconds{ static_cast<int>(std::floor(survivalSeconds)) };
		if (totalSeconds != displayedTimeSeconds)
		{
			displayedTimeSeconds = totalSeconds;
			std::ostringstream text;
			text << "TIME " << std::setfill('0') << std::setw(2)
				<< totalSeconds / 60 << ':' << std::setw(2)
				<< totalSeconds % 60;
			scoreText.setString(text.str());
			CenterScoreText();
		}
		return;
	}
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

void HUD::UpdateParts(float deltaTime)
{
	partsGlow.Update(deltaTime);
	const int currentParts{ session.GetDisplayedParts() };
	if (currentParts != displayedParts)
	{
		if (currentParts > displayedParts)
			partsPulseRemaining = PartsPulseDuration;
		displayedParts = currentParts;
		partsText.setString("PARTS: " + std::to_string(displayedParts));
		CenterPartsText();
		partsGlow.Invalidate();
	}
	partsPulseRemaining = std::max(0.f, partsPulseRemaining - deltaTime);
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

	const int percentage{ static_cast<int>(std::round(
		ratio * 100.f * session.GetArmorMultiplier())) };
	healthText.setString("ARMOR " + std::to_string(percentage) + "%");
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
	shieldText.setPosition(shieldFrame.getPosition() + FrameSize * 0.5f);
}

void HUD::CenterHomingText()
{
	const sf::FloatRect bounds{ homingText.getLocalBounds() };
	homingText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f });
	homingText.setPosition(homingFrame.getPosition() + FrameSize * 0.5f);
}

void HUD::CenterWeaponText()
{
	const sf::FloatRect bounds{ weaponText.getLocalBounds() };
	weaponText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f });
	weaponText.setPosition(weaponFrame.getPosition() + FrameSize * 0.5f);
}

void HUD::CenterTimeSlowdownText()
{
	const sf::FloatRect bounds{ timeSlowdownText.getLocalBounds() };
	timeSlowdownText.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f });
	timeSlowdownText.setPosition(timeSlowdownFrame.getPosition() + FrameSize * 0.5f);
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

void HUD::CenterPartsText()
{
	const sf::FloatRect bounds{ partsText.getLocalBounds() };
	partsText.setOrigin({
		bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f
	});
	partsText.setPosition(PartsPanelPosition + ScorePanelSize * 0.5f + sf::Vector2f{ 28.f, 0.f });
}

void HUD::DrawScorePanel(sf::RenderTarget& target, const sf::RenderStates& states) const
{
	target.draw(scorePanel, states);
	target.draw(scoreText, states);
}

void HUD::DrawPartsPanel(sf::RenderTarget& target, const sf::RenderStates& states) const
{
	target.draw(partsPanel, states);
	target.draw(partsIcon, states);
	target.draw(partsText, states);
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
	if (partsVisible &&
		(partsPulseRemaining > 0.f || tutorialPartsHighlightRemaining > 0.f))
	{
		const float normalized{ partsPulseRemaining / PartsPulseDuration };
		const float tutorialFlash{ tutorialPartsHighlightRemaining > 0.f
			? 0.4f + 0.6f * std::abs(std::sin(tutorialPartsHighlightRemaining * 9.f))
			: 0.f };
		const float blink{ std::max(normalized *
			(0.62f + 0.38f * std::abs(std::sin(normalized * 4.f * std::numbers::pi_v<float>))),
			tutorialFlash) };
		const sf::Color flashColor{ LerpColor(
			sf::Color::Black, sf::Color(255, 190, 38), blink) };
		partsGlow.DrawBloom(
			target,
			partsPanel.getGlobalBounds(),
			[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				DrawPartsPanel(glowTarget, states);
			},
			flashColor,
			false);
	}
	if (partsVisible)
		DrawPartsPanel(target, sf::RenderStates::Default);
	if (partsVisible &&
		(partsPulseRemaining > 0.f || tutorialPartsHighlightRemaining > 0.f))
	{
		const float normalized{ partsPulseRemaining / PartsPulseDuration };
		const float tutorialFlash{ tutorialPartsHighlightRemaining > 0.f
			? 0.4f + 0.6f * std::abs(std::sin(tutorialPartsHighlightRemaining * 9.f))
			: 0.f };
		const float blink{ std::max(normalized *
			(0.62f + 0.38f * std::abs(std::sin(normalized * 4.f * std::numbers::pi_v<float>))),
			tutorialFlash) };
		partsGlow.DrawHighlight(
			target,
			partsPanel.getGlobalBounds(),
			LerpColor(sf::Color::Black, sf::Color(255, 215, 78), blink));
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

	if (homingVisible)
	{
		target.draw(homingFrame);
		if (homingFill.getTextureRect().size.x > 0)
		{
			homingGlow.DrawBloom(
				target,
				homingFill.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					sf::Sprite glowSource{ homingFill };
					glowSource.setColor(sf::Color::White);
					glowTarget.draw(glowSource, states);
				},
				homingFill.getColor(),
				false);
		}
		target.draw(homingFill);
		target.draw(homingText);
	}

	if (weaponVisible)
	{
		target.draw(weaponFrame);
		if (weaponFill.getTextureRect().size.x > 0)
		{
			weaponGlow.DrawBloom(
				target,
				weaponFill.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					sf::Sprite glowSource{ weaponFill };
					glowSource.setColor(sf::Color::White);
					glowTarget.draw(glowSource, states);
				},
				weaponFill.getColor(),
				false);
		}
		target.draw(weaponFill);
		target.draw(weaponText);
	}

	if (timeSlowdownVisible)
	{
		target.draw(timeSlowdownFrame);
		if (timeSlowdownFill.getTextureRect().size.x > 0)
		{
			timeSlowdownGlow.DrawBloom(
				target,
				timeSlowdownFill.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					sf::Sprite glowSource{ timeSlowdownFill };
					glowSource.setColor(sf::Color::White);
					glowTarget.draw(glowSource, states);
				},
				timeSlowdownFill.getColor(),
				false);
		}
		target.draw(timeSlowdownFill);
		target.draw(timeSlowdownText);
	}
}
