#include "HUD.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numbers>
#include <sstream>
#include <string>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/Assets.h"
#include "gameplay/GameplaySession.h"
#include "localization/LocalizationManager.h"
#include "ui/HealthColor.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace UI
{
	namespace
	{
		constexpr float HealthBarScale = 0.5f;
		constexpr float ScorePanelScale = 0.5f;
		constexpr sf::Vector2f ScorePanelPosition = { 20.f, 20.f };
		constexpr sf::Vector2f PartsPanelPosition = { 1513.f, 986.f };
		constexpr sf::Vector2f ScorePanelSize = { 387.f, 74.f };
		constexpr sf::Vector2f FramePosition = { 20.f, 1010.f };
		constexpr float BonusBarSpacing = 60.f;
		constexpr sf::Vector2f FillOffset = { 27.f * HealthBarScale, 19.f * HealthBarScale };
		constexpr sf::Vector2f FrameSize = { 640.f * HealthBarScale, 100.f * HealthBarScale };

		// Bar fills shrink/grow continuously (health, timers), and Rendering::NeonGlow
		// rebuilds its blur render textures whenever the bounds passed to it
		// change size at all. Quantizing the width to whole steps means the
		// glow only rebuilds when the visible size actually crosses a step
		// boundary -- a handful of times, not every single frame -- while the
		// drawn content and its position stay exactly as before.
		constexpr float GlowSizeStep = 8.f;

		sf::FloatRect QuantizedGlowBounds(const sf::Sprite& fillSprite)
		{
			sf::FloatRect bounds = fillSprite.getGlobalBounds();
			bounds.size.x = std::ceil(bounds.size.x / GlowSizeStep) * GlowSizeStep;
			return bounds;
		}

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

	HUD::HUD(Assets& assets, const GameplaySession& session, LocalizationManager& localize)
		: assets(assets)
		, session(session)
		, localization(localize)

		, scoreText(assets.Fonts().Get(localize.GetRegularFont()))
		, scorePanel(assets.Textures().Get(Config::Texture::ScorePanelFrame))
		, scoreGlowEffect(assets)

		, partsText(assets.Fonts().Get(localize.GetRegularFont()))
		, partsPanel(assets.Textures().Get(Config::Texture::ScorePanelFrame))
		, partsIcon(assets.Textures().Get(Config::Texture::ShipUpgradesPartsIcon))
		, partsGlowEffect(assets)

		, healthText(assets.Fonts().Get(localize.GetBoldFont()))
		, healthFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
		, healthFill(assets.Textures().Get(Config::Texture::HealthBarFill))
		, healthGlowEffect(assets)

		, shieldText(assets.Fonts().Get(localize.GetBoldFont()))
		, shieldFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
		, shieldFill(assets.Textures().Get(Config::Texture::HealthBarFill))
		, shieldGlowEffect(assets)

		, homingText(assets.Fonts().Get(localize.GetBoldFont()))
		, homingFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
		, homingFill(assets.Textures().Get(Config::Texture::HealthBarFill))
		, homingGlowEffect(assets)

		, weaponText(assets.Fonts().Get(localize.GetBoldFont()))
		, weaponFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
		, weaponFill(assets.Textures().Get(Config::Texture::HealthBarFill))
		, weaponGlowEffect(assets)

		, timeSlowdownText(assets.Fonts().Get(localize.GetBoldFont()))
		, timeSlowdownFrame(assets.Textures().Get(Config::Texture::HealthBarFrame))
		, timeSlowdownFill(assets.Textures().Get(Config::Texture::HealthBarFill))
		, timeSlowdownGlowEffect(assets)
	{
		scorePanel.setPosition(ScorePanelPosition);
		scorePanel.setScale({ ScorePanelScale, ScorePanelScale });

		scoreText.setCharacterSize(28);
		scoreText.setLetterSpacing(1.04f);
		scoreText.setFillColor(sf::Color(226, 249, 255));
		scoreText.setOutlineColor(sf::Color(4, 24, 38, 230));
		scoreText.setOutlineThickness(2.f);

		displayedScore = session.GetScore();

		scoreText.setString(localization.FormatText("hud.score", "value", std::to_string(displayedScore)));
		CenterScoreText();

		partsPanel.setPosition(PartsPanelPosition);
		partsPanel.setScale({ ScorePanelScale, ScorePanelScale });

		const sf::Vector2u partTextureSize = partsIcon.getTexture().getSize();
		const float partIconScale = 48.f / static_cast<float>(std::max(partTextureSize.x, partTextureSize.y));

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

		partsText.setString(localization.FormatText("hud.parts", "value", std::to_string(displayedParts)));
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
		// scoreText/partsText only reformat when their underlying value changes,
		// so a language switch alone (e.g. from the pause menu) wouldn't update
		// their label word until the next score/parts change. Forcing the
		// "last known value" sentinels to mismatch makes UpdateScore/UpdateParts
		// reformat through their normal path instead of duplicating it here.
		if (localizationRevision.Update(localization))
		{
			displayedScore = -1;
			displayedParts = -1;
			displayedTimeSeconds = -1;
			RefreshLocalizedFonts();
		}

		tutorialScoreHighlightRemaining = std::max(0.f, tutorialScoreHighlightRemaining - deltaTime);
		tutorialHealthHighlightRemaining = std::max(0.f, tutorialHealthHighlightRemaining - deltaTime);
		tutorialShieldHighlightRemaining = std::max(0.f, tutorialShieldHighlightRemaining - deltaTime);
		tutorialPartsHighlightRemaining = std::max(0.f, tutorialPartsHighlightRemaining - deltaTime);

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
		scoreGlowEffect.Invalidate();
	}

	void HUD::HighlightHealth(float duration) noexcept
	{
		tutorialHealthHighlightRemaining = std::max(tutorialHealthHighlightRemaining, duration);
		healthGlowEffect.Invalidate();
	}

	void HUD::HighlightShield(float duration) noexcept
	{
		tutorialShieldHighlightRemaining = std::max(tutorialShieldHighlightRemaining, duration);
		shieldGlowEffect.Invalidate();
	}

	void HUD::SetRunMode(bool needToEnableRunMode) noexcept
	{
		isRunMode = needToEnableRunMode;
		displayedTimeSeconds = -1;

		UpdateScore(0.f);
	}

	void HUD::SetPartsVisible(bool needToShowParts) noexcept
	{
		isPartsVisible = needToShowParts;
	}

	void HUD::SetScoreVisible(bool needToShowScore) noexcept
	{
		isScoreVisible = needToShowScore;
	}

	void HUD::SetSurvivalTime(float seconds) noexcept
	{
		survivalSeconds = std::max(0.f, seconds);
	}

	void HUD::HighlightParts(float duration) noexcept
	{
		tutorialPartsHighlightRemaining = std::max(tutorialPartsHighlightRemaining, duration);
		partsGlowEffect.Invalidate();
	}

	void HUD::UpdateShieldBar(float deltaTime)
	{
		const Shield& shield = session.GetPlayerShield();

		isShieldVisible = shield.IsActive();
		if (!isShieldVisible)
		{
			shieldFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
			shieldBlinkTimer = 0.f;
			return;
		}

		shieldGlowEffect.Update(deltaTime);

		const float ratio = std::clamp(shield.GetRatio(), 0.f, 1.f);
		const sf::Vector2u textureSize = shieldFill.getTexture().getSize();
		const int visibleWidth = static_cast<int>(std::round(textureSize.x * ratio));

		shieldFill.setTextureRect(sf::IntRect({ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));

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

		const int percentage = static_cast<int>(std::ceil(ratio * 100.f));

		shieldText.setString(localization.FormatText("hud.shield", "value", std::to_string(percentage)));
	}

	void HUD::UpdateHomingBar(float deltaTime)
	{
		isHomingVisible = session.IsHomingBulletsActive();
		if (!isHomingVisible)
		{
			homingFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
			return;
		}

		homingGlowEffect.Update(deltaTime);

		const float ratio = std::clamp(session.GetHomingBulletsRatio(), 0.f, 1.f);
		const sf::Vector2u textureSize = homingFill.getTexture().getSize();
		const int visibleWidth = static_cast<int>(std::round(textureSize.x * ratio));

		homingFill.setTextureRect(sf::IntRect({ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));
		homingFill.setColor(sf::Color(255, 190, 40));

		const int percentage = static_cast<int>(std::ceil(ratio * 100.f));

		homingText.setString(localization.FormatText("hud.homing", "value", std::to_string(percentage)));
	}

	void HUD::UpdateWeaponBar(float deltaTime)
	{
		using WeaponMode = GameplaySession::WeaponMode;

		const WeaponMode mode = session.GetWeaponMode();

		isWeaponVisible = mode != WeaponMode::Normal;
		if (!isWeaponVisible)
		{
			weaponFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
			return;
		}

		weaponGlowEffect.Update(deltaTime);

		const float ratio = std::clamp(session.GetWeaponBonusRatio(), 0.f, 1.f);
		const sf::Vector2u textureSize = weaponFill.getTexture().getSize();
		const int visibleWidth = static_cast<int>(std::round(textureSize.x * ratio));

		weaponFill.setTextureRect(sf::IntRect({ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));

		const int percentage = static_cast<int>(std::ceil(ratio * 100.f));

		if (mode == WeaponMode::Laser)
		{
			weaponFill.setColor(sf::Color(255, 55, 28));
			weaponText.setFillColor(sf::Color(255, 218, 200));
			weaponText.setString(localization.FormatText("hud.laser", "value", std::to_string(percentage)));
		}
		else
		{
			weaponFill.setColor(sf::Color(255, 170, 30));
			weaponText.setFillColor(sf::Color(255, 238, 185));
			weaponText.setString(localization.FormatText("hud.triple", "value", std::to_string(percentage)));
		}
	}

	void HUD::UpdateTimeSlowdownBar(float deltaTime)
	{
		isTimeSlowdownVisible = session.IsTimeSlowdownActive();
		if (!isTimeSlowdownVisible)
		{
			timeSlowdownFill.setTextureRect(sf::IntRect({ 0, 0 }, { 0, 0 }));
			return;
		}

		timeSlowdownGlowEffect.Update(deltaTime);

		const float ratio = std::clamp(session.GetTimeSlowdownRatio(), 0.f, 1.f);
		const sf::Vector2u textureSize = timeSlowdownFill.getTexture().getSize();
		const int visibleWidth = static_cast<int>(std::round(textureSize.x * ratio));

		timeSlowdownFill.setTextureRect(sf::IntRect({ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));
		timeSlowdownFill.setColor(sf::Color(180, 75, 255));

		const int percentage = static_cast<int>(std::ceil(ratio * 100.f));
		timeSlowdownText.setString(localization.FormatText("hud.time_slow", "value", std::to_string(percentage)));
	}

	void HUD::UpdateBonusBarLayout()
	{
		float nextY = FramePosition.y - BonusBarSpacing;

		if (isShieldVisible)
		{
			shieldFrame.setPosition({ FramePosition.x, nextY });
			shieldFill.setPosition(shieldFrame.getPosition() + FillOffset);
			CenterShieldText();

			nextY -= BonusBarSpacing;
		}

		if (isHomingVisible)
		{
			homingFrame.setPosition({ FramePosition.x, nextY });
			homingFill.setPosition(homingFrame.getPosition() + FillOffset);
			CenterHomingText();

			nextY -= BonusBarSpacing;
		}

		if (isWeaponVisible)
		{
			weaponFrame.setPosition({ FramePosition.x, nextY });
			weaponFill.setPosition(weaponFrame.getPosition() + FillOffset);
			CenterWeaponText();

			nextY -= BonusBarSpacing;
		}

		if (isTimeSlowdownVisible)
		{
			timeSlowdownFrame.setPosition({ FramePosition.x, nextY });
			timeSlowdownFill.setPosition(timeSlowdownFrame.getPosition() + FillOffset);
			CenterTimeSlowdownText();
		}
	}

	void HUD::UpdateScore(float deltaTime)
	{
		scoreGlowEffect.Update(deltaTime);

		if (isRunMode)
		{
			const int totalSeconds = static_cast<int>(std::floor(survivalSeconds));
			if (totalSeconds != displayedTimeSeconds)
			{
				displayedTimeSeconds = totalSeconds;

				std::ostringstream text;
				text << std::setfill('0') << std::setw(2) << totalSeconds / 60 << ':' << std::setw(2) << totalSeconds % 60;

				scoreText.setString(localization.FormatText("hud.time", "value", text.str()));
				CenterScoreText();

				scoreGlowEffect.Invalidate();
			}

			return;
		}

		const int currentScore = session.GetScore();
		if (currentScore != displayedScore)
		{
			if (currentScore > displayedScore)
				scorePulseRemaining = ScorePulseDuration;

			displayedScore = currentScore;

			scoreText.setString(localization.FormatText("hud.score", "value", std::to_string(displayedScore)));
			CenterScoreText();

			scoreGlowEffect.Invalidate();
		}

		scorePulseRemaining = std::max(0.f, scorePulseRemaining - deltaTime);
	}

	void HUD::UpdateParts(float deltaTime)
	{
		partsGlowEffect.Update(deltaTime);

		const int currentParts = session.GetDisplayedParts();
		if (currentParts != displayedParts)
		{
			if (currentParts > displayedParts)
				partsPulseRemaining = PartsPulseDuration;

			displayedParts = currentParts;

			partsText.setString(localization.FormatText("hud.parts", "value", std::to_string(displayedParts)));
			CenterPartsText();

			partsGlowEffect.Invalidate();
		}

		partsPulseRemaining = std::max(0.f, partsPulseRemaining - deltaTime);
	}

	void HUD::UpdateHealthBar(float deltaTime)
	{
		const Health& health = session.GetPlayerHealth();
		const float ratio = std::clamp(health.GetRatio(), 0.f, 1.f);
		const sf::Vector2u textureSize = healthFill.getTexture().getSize();
		const int visibleWidth = static_cast<int>(std::round(textureSize.x * ratio));

		healthFill.setTextureRect(sf::IntRect({ 0, 0 }, { visibleWidth, static_cast<int>(textureSize.y) }));

		sf::Color fillColor{ GetHealthColor(ratio) };

		if (ratio > CriticalThreshold)
		{
			isCriticalWarningArmed = true;
			criticalWarningRemaining = 0.f;
			blinkTimer = 0.f;
		}
		else if (isCriticalWarningArmed && health.GetCurrent() > 0)
		{
			isCriticalWarningArmed = false;
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

		const int percentage = static_cast<int>(std::round(ratio * 100.f * session.GetArmorMultiplier()));

		healthText.setString(localization.FormatText("hud.armor", "value", std::to_string(percentage)));
		CenterHealthText();
	}

	void HUD::RefreshLocalizedFonts()
	{
		scoreText.setFont(assets.Fonts().Get(localization.GetRegularFont()));
		partsText.setFont(assets.Fonts().Get(localization.GetRegularFont()));
		healthText.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		shieldText.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		homingText.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		weaponText.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		timeSlowdownText.setFont(assets.Fonts().Get(localization.GetBoldFont()));

		scoreGlowEffect.Invalidate();
		partsGlowEffect.Invalidate();
		healthGlowEffect.Invalidate();
		shieldGlowEffect.Invalidate();
		homingGlowEffect.Invalidate();
		weaponGlowEffect.Invalidate();
		timeSlowdownGlowEffect.Invalidate();
	}

	void HUD::CenterHealthText()
	{
		TextLayout::CenterText(healthText, FramePosition + FrameSize * 0.5f);
	}

	void HUD::CenterShieldText()
	{
		TextLayout::CenterText(shieldText, shieldFrame.getPosition() + FrameSize * 0.5f);
	}

	void HUD::CenterHomingText()
	{
		TextLayout::CenterText(homingText, homingFrame.getPosition() + FrameSize * 0.5f);
	}

	void HUD::CenterWeaponText()
	{
		TextLayout::CenterText(weaponText, weaponFrame.getPosition() + FrameSize * 0.5f);
	}

	void HUD::CenterTimeSlowdownText()
	{
		TextLayout::CenterText(timeSlowdownText, timeSlowdownFrame.getPosition() + FrameSize * 0.5f);
	}

	void HUD::CenterScoreText()
	{
		TextLayout::CenterText(scoreText, ScorePanelPosition + ScorePanelSize * 0.5f);
	}

	void HUD::CenterPartsText()
	{
		TextLayout::CenterText(partsText, PartsPanelPosition + ScorePanelSize * 0.5f + sf::Vector2f{ 28.f, 0.f });
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

	void HUD::DrawResourceBar(sf::RenderTarget& target, Rendering::NeonGlow& glowEffect,
		const sf::Sprite& frame, const sf::Sprite& fill, const sf::Text& text) const
	{
		target.draw(frame);

		if (fill.getTextureRect().size.x > 0)
		{
			glowEffect.DrawBloom(target, QuantizedGlowBounds(fill),
				[&fill](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					sf::Sprite glowSource = fill;
					glowSource.setColor(sf::Color::White);
					glowTarget.draw(glowSource, states);
				},
				fill.getColor(),
				false);
		}

		// The frame texture also contains the opaque dark backing. Draw the fill on
		// top of it so that the backing cannot hide the changing health amount.
		target.draw(fill);
		target.draw(text);
	}

	void HUD::DrawResourceBarTutorialHighlight(sf::RenderTarget& target, Rendering::NeonGlow& glowEffect,
		const sf::Sprite& frame, const sf::Sprite& fill, const sf::Text& text,
		float remainingSeconds, sf::Color highlightColor) const
	{
		if (remainingSeconds <= 0.f)
			return;

		const float flash = 0.35f + 0.65f * std::abs(std::sin(remainingSeconds * 9.f));

		glowEffect.DrawBloom(target, frame.getGlobalBounds(),
			[&frame, &fill, &text](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				glowTarget.draw(frame, states);
				glowTarget.draw(fill, states);
				glowTarget.draw(text, states);
			},
			LerpColor(sf::Color::Black, highlightColor, flash),
			false);
	}

	void HUD::Draw(sf::RenderTarget& target)
	{
		const bool isScoreFlashing = scorePulseRemaining > 0.f || tutorialScoreHighlightRemaining > 0.f;
		if (isScoreVisible && isScoreFlashing)
		{
			const float normalized = scorePulseRemaining / ScorePulseDuration;
			const float tutorialFlash = tutorialScoreHighlightRemaining > 0.f
				? 0.35f + 0.65f * std::abs(std::sin(tutorialScoreHighlightRemaining * 9.f))
				: 0.f;
			const float flash = std::max(normalized * normalized, tutorialFlash);

			scoreGlowEffect.DrawBloom(target, scorePanel.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					DrawScorePanel(glowTarget, states);
				},
				LerpColor(sf::Color::Black, sf::Color(170, 250, 255), flash),
				false);

			DrawScorePanel(target, sf::RenderStates::Default);

			scoreGlowEffect.DrawHighlight(target, scorePanel.getGlobalBounds(),
				LerpColor(sf::Color::Black, sf::Color(205, 255, 255), flash));
		}
		else if (isScoreVisible)
		{
			DrawScorePanel(target, sf::RenderStates::Default);
		}

		const bool isPartsFlashing = partsPulseRemaining > 0.f || tutorialPartsHighlightRemaining > 0.f;
		if (isPartsVisible && isPartsFlashing)
		{
			const float normalized = partsPulseRemaining / PartsPulseDuration;
			const float tutorialFlash = tutorialPartsHighlightRemaining > 0.f
				? 0.4f + 0.6f * std::abs(std::sin(tutorialPartsHighlightRemaining * 9.f))
				: 0.f;

			const float oscillationPhase = normalized * 4.f * std::numbers::pi_v<float>;
			const float oscillationIntensity = std::abs(std::sin(oscillationPhase));
			const float pulseBrightness = 0.62f + 0.38f * oscillationIntensity;
			const float blink = std::max(normalized * pulseBrightness, tutorialFlash);

			partsGlowEffect.DrawBloom(target, partsPanel.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					DrawPartsPanel(glowTarget, states);
				},
				LerpColor(sf::Color::Black, sf::Color(255, 190, 38), blink),
				false);

			DrawPartsPanel(target, sf::RenderStates::Default);

			partsGlowEffect.DrawHighlight(target, partsPanel.getGlobalBounds(),
				LerpColor(sf::Color::Black, sf::Color(255, 215, 78), blink));
		}
		else if (isPartsVisible)
		{
			DrawPartsPanel(target, sf::RenderStates::Default);
		}

		DrawResourceBarTutorialHighlight(target, healthGlowEffect, healthFrame, healthFill, healthText,
			tutorialHealthHighlightRemaining, sf::Color(100, 255, 170));
		DrawResourceBar(target, healthGlowEffect, healthFrame, healthFill, healthText);

		if (isShieldVisible)
		{
			DrawResourceBarTutorialHighlight(target, shieldGlowEffect, shieldFrame, shieldFill, shieldText,
				tutorialShieldHighlightRemaining, sf::Color(80, 245, 255));
			DrawResourceBar(target, shieldGlowEffect, shieldFrame, shieldFill, shieldText);
		}

		if (isHomingVisible)
			DrawResourceBar(target, homingGlowEffect, homingFrame, homingFill, homingText);

		if (isWeaponVisible)
			DrawResourceBar(target, weaponGlowEffect, weaponFrame, weaponFill, weaponText);

		if (isTimeSlowdownVisible)
			DrawResourceBar(target, timeSlowdownGlowEffect, timeSlowdownFrame, timeSlowdownFill, timeSlowdownText);
	}
}