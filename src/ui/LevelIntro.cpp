#include "LevelIntro.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/Assets.h"
#include "localization/LocalizationManager.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr float FadeInDuration{ 0.42f };
	constexpr float HoldDuration{ 1.35f };
	constexpr float FadeOutDuration{ 0.55f };
	constexpr float TotalDuration{ FadeInDuration + HoldDuration + FadeOutDuration };
	constexpr sf::Vector2f PanelSize{ 1120.f, 330.f };
	constexpr sf::Color Cyan{ 25, 220, 255 };
	constexpr sf::Color Amber{ 255, 178, 42 };

	float SmoothStep(float value)
	{
		value = std::clamp(value, 0.f, 1.f);
		return value * value * (3.f - 2.f * value);
	}

	std::uint8_t ToAlpha(float value)
	{
		return static_cast<std::uint8_t>(
			std::clamp(value, 0.f, 1.f) * 255.f);
	}
}

LevelIntro::LevelIntro(Assets& assets, LocalizationManager& localize, sf::Vector2f screenSize)
	: levelGlow(assets), assets(assets), localization(localize)
	, titleGlow(assets)
	, shade(screenSize)
	, panel(PanelSize, 28.f, 12u)
	, upperLine({ 760.f, 3.f })
	, lowerLine({ 420.f, 2.f })
	, levelLabel(assets.Fonts().Get(localize.GetBoldFont()), "", 72u)
	, title(assets.Fonts().Get(localize.GetRegularFont()), "", 46u)
	, logicalSize(screenSize)
{
	shade.setFillColor(sf::Color::Transparent);
	panel.setPosition({
		(logicalSize.x - PanelSize.x) * 0.5f,
		(logicalSize.y - PanelSize.y) * 0.5f });
	panel.setFillColor(sf::Color(2, 10, 24, 230));
	panel.setOutlineThickness(2.f);
	panel.setOutlineColor(Cyan);

	upperLine.setOrigin({ upperLine.getSize().x * 0.5f, 1.5f });
	upperLine.setPosition({ logicalSize.x * 0.5f, logicalSize.y * 0.5f - 88.f });
	lowerLine.setOrigin({ lowerLine.getSize().x * 0.5f, 1.f });
	lowerLine.setPosition({ logicalSize.x * 0.5f, logicalSize.y * 0.5f + 92.f });

	levelLabel.setOutlineThickness(4.f);
	levelLabel.setLetterSpacing(1.12f);
	title.setOutlineThickness(3.f);
	title.setLetterSpacing(1.18f);
	Reset();
}

void LevelIntro::Start(int levelNumber)
{
	levelLabel.setFont(assets.Fonts().Get(localization.GetBoldFont()));
	title.setFont(assets.Fonts().Get(localization.GetRegularFont()));
	titleGlowEnabled = true;
	StartWithText(localization.FormatText("intro.level", "value", std::to_string(levelNumber)),
		localization.GetText("levels.title_" + std::to_string(levelNumber)));
}

void LevelIntro::StartMode(const sf::String& modeName, const sf::String& objective)
{
	levelLabel.setFont(assets.Fonts().Get(localization.GetBoldFont()));
	title.setFont(assets.Fonts().Get(localization.GetRegularFont(false)));
	titleGlowEnabled = false;
	StartWithText(modeName, objective);
}

void LevelIntro::StartWithText(const sf::String& heading, const sf::String& subtitle)
{
	levelLabel.setString(heading);
	title.setString(subtitle);
	TextLayout::FitWidth(levelLabel, PanelSize.x - 100.f, 42u);
	TextLayout::FitWidth(title, PanelSize.x - 120.f, 28u);
	elapsed = 0.f;
	active = true;
	levelGlow.Invalidate();
	titleGlow.Invalidate();
	ApplyAnimation();
}

bool LevelIntro::Update(float deltaTime)
{
	if (!active)
		return false;
	levelGlow.Update(deltaTime);
	titleGlow.Update(deltaTime);
	elapsed = std::min(TotalDuration, elapsed + deltaTime);
	ApplyAnimation();
	if (elapsed < TotalDuration)
		return false;
	Reset();
	return true;
}

void LevelIntro::Draw(sf::RenderTarget& target)
{
	if (!active)
		return;
	target.draw(shade);
	target.draw(panel);
	target.draw(upperLine);
	target.draw(lowerLine);

	levelGlow.DrawBloom(target, levelLabel.getGlobalBounds(),
		[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
		{ glowTarget.draw(levelLabel, states); }, Cyan, false);
	if (titleGlowEnabled)
		titleGlow.DrawBloom(target, title.getGlobalBounds(),
			[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{ glowTarget.draw(title, states); }, Amber, false);
	target.draw(levelLabel);
	target.draw(title);
}

void LevelIntro::Reset() noexcept
{
	active = false;
	elapsed = 0.f;
	shade.setFillColor(sf::Color::Transparent);
	panel.setFillColor(sf::Color::Transparent);
	panel.setOutlineColor(sf::Color::Transparent);
	upperLine.setFillColor(sf::Color::Transparent);
	lowerLine.setFillColor(sf::Color::Transparent);
	levelLabel.setFillColor(sf::Color::Transparent);
	levelLabel.setOutlineColor(sf::Color::Transparent);
	title.setFillColor(sf::Color::Transparent);
	title.setOutlineColor(sf::Color::Transparent);
}

bool LevelIntro::IsActive() const noexcept
{
	return active;
}

void LevelIntro::ApplyAnimation()
{
	const float fadeIn{ SmoothStep(elapsed / FadeInDuration) };
	const float fadeOutStart{ FadeInDuration + HoldDuration };
	const float fadeOut{ elapsed <= fadeOutStart
		? 1.f
		: 1.f - SmoothStep((elapsed - fadeOutStart) / FadeOutDuration) };
	const float opacity{ fadeIn * fadeOut };
	const auto alpha{ ToAlpha(opacity) };

	shade.setFillColor(sf::Color(0, 3, 12, ToAlpha(opacity * 0.82f)));
	panel.setFillColor(sf::Color(2, 10, 24, ToAlpha(opacity * 0.94f)));
	panel.setOutlineColor(sf::Color(Cyan.r, Cyan.g, Cyan.b, alpha));
	upperLine.setFillColor(sf::Color(Cyan.r, Cyan.g, Cyan.b, alpha));
	lowerLine.setFillColor(sf::Color(Amber.r, Amber.g, Amber.b, alpha));
	levelLabel.setFillColor(sf::Color(220, 250, 255, alpha));
	levelLabel.setOutlineColor(sf::Color(0, 50, 75, alpha));
	title.setFillColor(sf::Color(255, 226, 160, alpha));
	title.setOutlineColor(sf::Color(70, 38, 0, alpha));

	const float rise{ 22.f * (1.f - fadeIn) };
	CenterText(levelLabel, { logicalSize.x * 0.5f, logicalSize.y * 0.5f - 22.f + rise });
	CenterText(title, { logicalSize.x * 0.5f, logicalSize.y * 0.5f + 53.f + rise });
	levelGlow.Invalidate();
	titleGlow.Invalidate();
}

void LevelIntro::CenterText(sf::Text& text, sf::Vector2f position)
{
	const sf::FloatRect bounds{ text.getLocalBounds() };
	text.setOrigin({
		bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f });
	text.setPosition(position);
}
