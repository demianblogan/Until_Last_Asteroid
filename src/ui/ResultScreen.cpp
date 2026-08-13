#include "ResultScreen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "systems/GamepadManager.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr sf::Vector2f TitleFrameSize{ 1180.f, 203.f };
    constexpr sf::Vector2f ButtonSize{ 540.f, 104.f };
	constexpr sf::Vector2f StatisticsPanelSize{ 1320.f, 440.f };
	constexpr sf::Vector2f StatisticsPanelPosition{ 300.f, 315.f };
    constexpr float DarkenDuration{ 0.42f };
    constexpr float TitleStart{ 0.22f };
    constexpr float TitleDuration{ 0.28f };
	constexpr float StatisticsPanelStart{ 0.50f };
	constexpr float StatisticsPanelDuration{ 0.24f };
	constexpr float FirstStatisticStart{ 0.78f };
	constexpr float StatisticInterval{ 0.50f };
	constexpr float StatisticFadeDuration{ 0.18f };
	constexpr float ValueCountDuration{ 1.f };
	constexpr float FirstButtonStart{ 4.12f };
	constexpr float SecondButtonStart{ 4.28f };
	constexpr float InteractiveTime{ 4.42f };
    constexpr sf::Color SuccessGlowColor{ 65, 255, 90 };
    constexpr sf::Color SelectionGlowColor{ 255, 178, 42 };
	constexpr sf::Color StatisticsGold{ 255, 190, 72 };
	constexpr std::size_t StatisticLineCount{ 6u };

    float Progress(float elapsed, float start, float duration)
    {
        return std::clamp((elapsed - start) / duration, 0.f, 1.f);
    }

    std::uint8_t ToAlpha(float opacity)
    {
        return static_cast<std::uint8_t>(std::clamp(opacity, 0.f, 1.f) * 255.f);
    }

	std::string FormatTime(float seconds)
	{
		const int totalSeconds{ std::max(0, static_cast<int>(seconds + 0.5f)) };
		std::ostringstream stream;
		stream << std::setfill('0') << std::setw(2) << totalSeconds / 60
			<< ':' << std::setw(2) << totalSeconds % 60;
		return stream.str();
	}

	void AlignLeft(sf::Text& text, sf::Vector2f position)
	{
		const sf::FloatRect bounds{ text.getLocalBounds() };
		text.setOrigin({
			bounds.position.x,
			bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(position);
	}

	void AlignRight(sf::Text& text, sf::Vector2f position)
	{
		const sf::FloatRect bounds{ text.getLocalBounds() };
		text.setOrigin({
			bounds.position.x + bounds.size.x,
			bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(position);
	}
}

ResultScreen::ResultScreen(
    AssetStore& assets, AudioManager& gameAudio, GamepadManager& gamepadManager,
    sf::Vector2f screenSize)
    : audio(gameAudio)
    , gamepad(gamepadManager)
    , logicalSize(screenSize)
    , shade(screenSize)
    , titleFrame(assets.Textures().Get(Config::Texture::ResultTitleFrame))
    , title(assets.Fonts().Get(Config::Font::MenuSemibold), "LEVEL COMPLETE", 86u)
	, statisticsPanel(StatisticsPanelSize, 22.f, 12u)
	, statisticsSeparator({ StatisticsPanelSize.x - 100.f, 2.f })
	, statisticsTitle(assets.Fonts().Get(Config::Font::BodyRegular), "STATISTICS", 36u)
    , titleGlow(assets)
    , buttonGlow(assets)
    , menuCursor(assets, Config::Texture::MenuPointer, { 6.f, 2.f }, SelectionGlowColor)
{
    const sf::Vector2u textureSize{ titleFrame.getTexture().getSize() };
    titleFrame.setScale({
        TitleFrameSize.x / static_cast<float>(textureSize.x),
        TitleFrameSize.y / static_cast<float>(textureSize.y) });
	titleFrame.setPosition({ (logicalSize.x - TitleFrameSize.x) * 0.5f, 88.f });

    title.setFillColor(sf::Color(225, 255, 230));
    title.setOutlineColor(sf::Color(0, 92, 30));
    title.setOutlineThickness(4.f);
    title.setLetterSpacing(1.05f);
	CenterText(title, { logicalSize.x * 0.5f, 190.f });

	const sf::Font& regularFont{ assets.Fonts().Get(Config::Font::MenuRegular) };
	const sf::Font& bodyFont{ assets.Fonts().Get(Config::Font::BodyRegular) };
	statisticsPanel.setPosition(StatisticsPanelPosition);
	statisticsPanel.setFillColor(sf::Color(2, 13, 27, 230));
	statisticsPanel.setOutlineColor(sf::Color(25, 205, 240));
	statisticsPanel.setOutlineThickness(2.f);
	statisticsSeparator.setPosition({
		StatisticsPanelPosition.x + 50.f,
		StatisticsPanelPosition.y + 75.f });
	statisticsSeparator.setFillColor(StatisticsGold);
	statisticsTitle.setFillColor(StatisticsGold);
	statisticsTitle.setOutlineColor(sf::Color(70, 32, 2, 220));
	statisticsTitle.setOutlineThickness(2.f);
	CenterText(statisticsTitle, {
		logicalSize.x * 0.5f, StatisticsPanelPosition.y + 42.f });

	statisticLabels.reserve(StatisticLineCount);
	statisticValues.reserve(StatisticLineCount);
	for (std::size_t index{ 0u }; index < StatisticLineCount; ++index)
	{
		const unsigned int characterSize{ index >= 4u ? 31u : 27u };
		statisticLabels.emplace_back(bodyFont, "", characterSize);
		statisticValues.emplace_back(regularFont, "0", characterSize);
		statisticLabels.back().setOutlineThickness(1.5f);
		statisticValues.back().setOutlineThickness(1.5f);
	}

	const sf::Font& menuFont{ regularFont };
    const sf::Texture& idle{ assets.Textures().Get(Config::Texture::MenuButtonIdle) };
    const sf::Texture& selected{ assets.Textures().Get(Config::Texture::MenuButtonSelected) };
    buttons.reserve(2u);
    for (std::size_t index{ 0u }; index < 2u; ++index)
    {
        buttons.emplace_back(menuFont, idle, selected, "", ButtonSize);
        buttons.back().SetPosition({
            (logicalSize.x - ButtonSize.x) * 0.5f,
			785.f + static_cast<float>(index) * 120.f });
    }

    Reset();
}

void ResultScreen::Start(
	Mode newMode, int level, const Statistics& statistics)
{
    mode = newMode;
	this->statistics = statistics;
    active = true;
    interactive = false;
    elapsed = 0.f;
    selectedIndex = 0u;

    if (mode == Mode::Victory)
    {
        title.setString("VICTORY");
        buttons[0].SetLabel("Play Again");
    }
	else if (mode == Mode::LevelReplay)
	{
		title.setString("LEVEL " + std::to_string(level) + " COMPLETE");
		buttons[0].SetLabel("Return to Levels");
	}
    else
    {
        title.setString("LEVEL " + std::to_string(level) + " COMPLETE");
        buttons[0].SetLabel("Continue");
    }
	statisticLabels[0].setString("Combat score:");
	statisticLabels[1].setString(
		"Armor bonus (remaining " + std::to_string(statistics.armorPercent) + "%):");
	statisticLabels[2].setString(
		"Accuracy bonus (your " + std::to_string(statistics.accuracyPercent) +
		"% / target " + std::to_string(statistics.targetAccuracyPercent) + "%):");
	statisticLabels[3].setString(
		"Time bonus (your " + FormatTime(statistics.completionSeconds) +
		" / target " + FormatTime(statistics.targetSeconds) + "):");
	statisticLabels[4].setString("Level score:");
	statisticLabels[5].setString("Campaign score:");
	for (std::size_t index{ 0u }; index < statisticLabels.size(); ++index)
	{
		const float y{ StatisticsPanelPosition.y + 112.f +
			static_cast<float>(index) * 52.f };
		AlignLeft(statisticLabels[index], { StatisticsPanelPosition.x + 72.f, y });
		AlignRight(statisticValues[index], {
			StatisticsPanelPosition.x + StatisticsPanelSize.x - 72.f, y });
	}
    buttons[1].SetLabel("Go to Main Menu");
	CenterText(title, { logicalSize.x * 0.5f, 190.f });
    Select(0u, false);
    titleGlow.Invalidate();
    buttonGlow.Invalidate();
    ApplyVisualState();
    audio.PlaySound(Config::Sound::InterfaceActivation, SoundGroup::UI, 80.f, 1.f,
        SoundPlayback::Restart);
}

void ResultScreen::Reset()
{
    active = false;
    interactive = false;
    elapsed = 0.f;
    shade.setFillColor(sf::Color::Transparent);
    titleFrame.setColor(sf::Color::Transparent);
    title.setFillColor(sf::Color::Transparent);
    title.setOutlineColor(sf::Color::Transparent);
	statisticsPanel.setFillColor(sf::Color::Transparent);
	statisticsPanel.setOutlineColor(sf::Color::Transparent);
	statisticsSeparator.setFillColor(sf::Color::Transparent);
	statisticsTitle.setFillColor(sf::Color::Transparent);
	statisticsTitle.setOutlineColor(sf::Color::Transparent);
	for (sf::Text& line : statisticLabels)
	{
		line.setFillColor(sf::Color::Transparent);
		line.setOutlineColor(sf::Color::Transparent);
	}
	for (sf::Text& value : statisticValues)
	{
		value.setFillColor(sf::Color::Transparent);
		value.setOutlineColor(sf::Color::Transparent);
	}
}

void ResultScreen::Update(float deltaTime)
{
    if (!active)
        return;
    titleGlow.Update(deltaTime);
    buttonGlow.Update(deltaTime);
    menuCursor.Update(deltaTime);
    elapsed = std::min(InteractiveTime, elapsed + deltaTime);
    interactive = elapsed >= InteractiveTime;
    ApplyVisualState();
}

std::optional<ResultScreen::Action> ResultScreen::HandleEvent(
    const sf::Event& event, sf::RenderWindow& window)
{
    if (!active)
        return std::nullopt;

    const auto navigation{ gamepad.GetNavigationAction(event) };
    if (!interactive)
    {
        if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>() ||
            navigation != GamepadManager::NavigationAction::None)
            SkipAnimation();
        return std::nullopt;
    }

    using enum GamepadManager::NavigationAction;
    switch (navigation)
    {
    case Up: SelectPrevious(); return std::nullopt;
    case Down: SelectNext(); return std::nullopt;
    case Confirm: return ActivateSelected();
    case Back: Select(1u, false); return ActivateSelected();
    default: break;
    }

    if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
    {
        if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W)
            SelectPrevious();
        else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S)
            SelectNext();
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
            return ActivateSelected();
        return std::nullopt;
    }

    if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
    {
        UpdateMouseSelection(window.mapPixelToCoords(moved->position));
        return std::nullopt;
    }
    if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() })
    {
        if (pressed->button != sf::Mouse::Button::Left)
            return std::nullopt;
        const sf::Vector2f position{ window.mapPixelToCoords(pressed->position) };
        for (std::size_t index{ 0u }; index < buttons.size(); ++index)
            if (buttons[index].Contains(position))
            {
                Select(index, false);
                return ActivateSelected();
            }
    }
    return std::nullopt;
}

void ResultScreen::Draw(sf::RenderTarget& target)
{
    if (!active)
        return;

    target.draw(shade);
    if (Progress(elapsed, TitleStart, TitleDuration) >= 1.f)
    {
        titleGlow.DrawBloom(target, titleFrame.getGlobalBounds(),
            [this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
            {
                glowTarget.draw(titleFrame, states);
                glowTarget.draw(title, states);
            }, SuccessGlowColor, false);
    }
    target.draw(titleFrame);
    target.draw(title);
	target.draw(statisticsPanel);
	target.draw(statisticsSeparator);
	target.draw(statisticsTitle);
	for (std::size_t index{ 0u }; index < statisticLabels.size(); ++index)
	{
		if (elapsed < FirstStatisticStart + static_cast<float>(index) * StatisticInterval)
			continue;
		const sf::Text& value{ statisticValues[index] };
		target.draw(statisticLabels[index]);
		target.draw(value);
	}

    const std::size_t visibleButtons{ elapsed >= SecondButtonStart
        ? 2u : (elapsed >= FirstButtonStart ? 1u : 0u) };
    if (interactive && visibleButtons > 0u)
    {
        const MenuButton& selected{ buttons[selectedIndex] };
        buttonGlow.DrawBloom(target, selected.GetBounds(),
            [&selected](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
            { selected.Draw(glowTarget, states); }, SelectionGlowColor);
    }
    for (std::size_t index{ 0u }; index < visibleButtons; ++index)
        buttons[index].Draw(target);
    if (interactive && visibleButtons > 0u)
        buttonGlow.DrawHighlight(target, buttons[selectedIndex].GetBounds(), SelectionGlowColor);
}

void ResultScreen::DrawCursor(sf::RenderWindow& window)
{
    if (active && interactive && !gamepad.IsUsingGamepad())
        menuCursor.Draw(window);
}

bool ResultScreen::IsActive() const noexcept { return active; }
ResultScreen::Mode ResultScreen::GetMode() const noexcept { return mode; }

void ResultScreen::SkipAnimation()
{
    elapsed = InteractiveTime;
    interactive = true;
    ApplyVisualState();
    Select(0u, false);
}

void ResultScreen::ApplyVisualState()
{
    shade.setFillColor(sf::Color(0, 3, 12,
        ToAlpha(Progress(elapsed, 0.f, DarkenDuration) * 0.76f)));
    const auto titleAlpha{ ToAlpha(Progress(elapsed, TitleStart, TitleDuration)) };
    titleFrame.setColor(sf::Color(255, 255, 255, titleAlpha));
    title.setFillColor(sf::Color(225, 255, 230, titleAlpha));
    title.setOutlineColor(sf::Color(0, 92, 30, titleAlpha));
	const auto panelAlpha{ ToAlpha(Progress(
		elapsed, StatisticsPanelStart, StatisticsPanelDuration)) };
	statisticsPanel.setFillColor(sf::Color(2, 13, 27,
		static_cast<std::uint8_t>(static_cast<float>(panelAlpha) * 0.90f)));
	statisticsPanel.setOutlineColor(sf::Color(25, 205, 240, panelAlpha));
	statisticsSeparator.setFillColor(sf::Color(
		StatisticsGold.r, StatisticsGold.g, StatisticsGold.b, panelAlpha));
	statisticsTitle.setFillColor(sf::Color(
		StatisticsGold.r, StatisticsGold.g, StatisticsGold.b, panelAlpha));
	statisticsTitle.setOutlineColor(sf::Color(70, 32, 2, panelAlpha));

	const std::array<int, StatisticLineCount> targetValues{
		statistics.combatScore,
		statistics.armorBonus,
		statistics.accuracyBonus,
		statistics.timeBonus,
		statistics.levelTotal,
		statistics.campaignTotal };
	for (std::size_t index{ 0u }; index < statisticLabels.size(); ++index)
	{
		const float rowStart{ FirstStatisticStart +
			static_cast<float>(index) * StatisticInterval };
		const auto rowAlpha{ ToAlpha(Progress(
			elapsed, rowStart, StatisticFadeDuration)) };
		const float countProgress{ Progress(
			elapsed, rowStart, ValueCountDuration) };
		const float easedProgress{ 1.f - std::pow(1.f - countProgress, 3.f) };
		const int displayedValue{ static_cast<int>(std::lround(
			static_cast<float>(targetValues[index]) * easedProgress)) };
		const std::string prefix{ index >= 1u && index <= 3u ? "+" : "" };
		statisticValues[index].setString(prefix + std::to_string(displayedValue));
		AlignRight(statisticValues[index], {
			StatisticsPanelPosition.x + StatisticsPanelSize.x - 72.f,
			StatisticsPanelPosition.y + 112.f + static_cast<float>(index) * 52.f });
		const sf::Color labelColor{ index >= 4u
			? sf::Color(155, 245, 255, rowAlpha)
			: sf::Color(225, 240, 246, rowAlpha) };
		statisticLabels[index].setFillColor(labelColor);
		statisticLabels[index].setOutlineColor(sf::Color(2, 14, 25, rowAlpha));
		statisticValues[index].setFillColor(sf::Color(215, 252, 255, rowAlpha));
		statisticValues[index].setOutlineColor(sf::Color(5, 95, 125, rowAlpha));
	}
}

void ResultScreen::Select(std::size_t index, bool playSound)
{
    const bool changed{ selectedIndex != index };
    selectedIndex = index;
    for (std::size_t i{ 0u }; i < buttons.size(); ++i)
        buttons[i].SetSelected(i == selectedIndex);
    if (changed)
        buttonGlow.Invalidate();
    if (changed && playSound)
        audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f,
            SoundPlayback::Restart);
}

void ResultScreen::SelectPrevious()
{
    Select(selectedIndex == 0u ? buttons.size() - 1u : selectedIndex - 1u);
}

void ResultScreen::SelectNext() { Select((selectedIndex + 1u) % buttons.size()); }

void ResultScreen::UpdateMouseSelection(sf::Vector2f position)
{
    for (std::size_t index{ 0u }; index < buttons.size(); ++index)
        if (buttons[index].Contains(position))
        {
            Select(index);
            return;
        }
}

std::optional<ResultScreen::Action> ResultScreen::ActivateSelected()
{
    audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f,
        SoundPlayback::Restart);
    return selectedIndex == 0u ? Action::Primary : Action::MainMenu;
}

void ResultScreen::CenterText(sf::Text& text, sf::Vector2f position)
{
    const sf::FloatRect bounds{ text.getLocalBounds() };
    text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f });
    text.setPosition(position);
}
