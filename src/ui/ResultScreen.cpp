#include "ResultScreen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <string>
#include <utility>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "ui/TextLayout.h"
#include "input/GamepadManager.h"
#include "utils/ConfigEnums.h"

namespace UI
{
	namespace
	{
		constexpr sf::Vector2f TitleFrameSize{ 1180.f, 203.f };
		constexpr sf::Vector2f ButtonSize{ 480.f, 88.f };
		constexpr float ButtonGap = 32.f;
		constexpr sf::Vector2f StatisticsPanelSize{ 1320.f, 480.f };
		constexpr sf::Vector2f StatisticsPanelPosition{ 300.f, 315.f };
		constexpr float DarkenDuration = 0.42f;
		constexpr float TitleStart = 0.22f;
		constexpr float TitleDuration = 0.28f;
		constexpr float StatisticsPanelStart = 0.50f;
		constexpr float StatisticsPanelDuration = 0.24f;
		constexpr float FirstStatisticStart = 0.78f;
		constexpr float StatisticInterval = 0.42f;
		constexpr float StatisticFadeDuration = 0.18f;
		constexpr float ValueCountDuration = 1.f;
		constexpr float FirstButtonStart = 3.55f;
		constexpr float SecondButtonStart = 3.70f;
		constexpr float ThirdButtonStart = 3.85f;
		constexpr float InteractiveTime = 4.0f;
		constexpr sf::Color SuccessGlowColor{ 65, 255, 90 };
		constexpr sf::Color SelectionGlowColor{ 255, 178, 42 };
		constexpr sf::Color StatisticsGold{ 255, 190, 72 };
		constexpr std::size_t StatisticLineCount{ 5u };

		constexpr std::array<sf::Vector2f, StatisticLineCount> LabelPositions =
		{
			sf::Vector2f{ 390.f, 455.f },
			sf::Vector2f{ 390.f, 515.f },
			sf::Vector2f{ 390.f, 575.f },
			sf::Vector2f{ 390.f, 635.f },
			sf::Vector2f{ 390.f, 715.f }
		};

		constexpr std::array<sf::Vector2f, StatisticLineCount> ValuePositions =
		{
			sf::Vector2f{ 1530.f, 455.f },
			sf::Vector2f{ 1530.f, 515.f },
			sf::Vector2f{ 1530.f, 575.f },
			sf::Vector2f{ 1530.f, 635.f },
			sf::Vector2f{ 1530.f, 715.f }
		};

		float Progress(float animationElapsedSeconds, float start, float duration)
		{
			return std::clamp((animationElapsedSeconds - start) / duration, 0.f, 1.f);
		}

		std::uint8_t ToAlpha(float opacity)
		{
			return static_cast<std::uint8_t>(std::clamp(opacity, 0.f, 1.f) * 255.f);
		}

		void AlignLeft(sf::Text& text, sf::Vector2f position)
		{
			const sf::FloatRect bounds{ text.getLocalBounds() };
			text.setOrigin({ bounds.position.x,	bounds.position.y + bounds.size.y * 0.5f });
			text.setPosition(position);
		}

		void AlignRight(sf::Text& text, sf::Vector2f position)
		{
			const sf::FloatRect bounds{ text.getLocalBounds() };
			text.setOrigin({ bounds.position.x + bounds.size.x, bounds.position.y + bounds.size.y * 0.5f });
			text.setPosition(position);
		}
	}

	ResultScreen::ResultScreen(Assets& assets, AudioManager& gameAudio, GamepadManager& gamepadManager,
		LocalizationManager& localizationManager, sf::Vector2f screenSize)
		: assets(assets)
		, audio(gameAudio)
		, gamepad(gamepadManager)
		, localization(localizationManager)
		, logicalSize(screenSize)
		, shade(screenSize)
		, titleFrame(assets.Textures().Get(Config::Texture::ResultTitleFrame))
		, title(assets.Fonts().Get(localizationManager.GetBoldFont()), "", 86u)
		, statisticsPanel(StatisticsPanelSize, 22.f, 12u)
		, statisticsSeparator({ StatisticsPanelSize.x - 100.f, 2.f })
		, statisticsTitle(
			assets.Fonts().Get(localizationManager.GetBoldFont()), localizationManager.GetText("results.statistics"), 36u)
		, titleGlow(assets)
		, buttonGlow(assets)
		, menuCursor(assets, Config::Texture::MenuPointer, { 6.f, 2.f }, SelectionGlowColor)
		, buttonList(audio, buttonGlow)
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
		TextLayout::CenterText(title, { logicalSize.x * 0.5f, 190.f });

		const sf::Font& regularFont = assets.Fonts().Get(localizationManager.GetRegularFont());
		const sf::Font& bodyFont = assets.Fonts().Get(localizationManager.GetRegularFont(false));

		statisticsPanel.setPosition(StatisticsPanelPosition);
		statisticsPanel.setFillColor(sf::Color(2, 13, 27, 230));
		statisticsPanel.setOutlineColor(sf::Color(25, 205, 240));
		statisticsPanel.setOutlineThickness(2.f);

		statisticsSeparator.setPosition({ StatisticsPanelPosition.x + 50.f,	StatisticsPanelPosition.y + 75.f });
		statisticsSeparator.setFillColor(StatisticsGold);

		statisticsTitle.setFillColor(StatisticsGold);
		statisticsTitle.setOutlineColor(sf::Color(70, 32, 2, 220));
		statisticsTitle.setOutlineThickness(2.f);

		TextLayout::CenterText(statisticsTitle, { logicalSize.x * .5f, StatisticsPanelPosition.y + 52.f });

		statisticLabels.reserve(StatisticLineCount);
		statisticValues.reserve(StatisticLineCount);

		for (std::size_t index = 0u; index < StatisticLineCount; index++)
		{
			const unsigned int characterSize = index == 4u ? 31u : 27u;

			statisticLabels.emplace_back(bodyFont, "", characterSize);
			statisticValues.emplace_back(regularFont, "0", characterSize);

			statisticLabels.back().setOutlineThickness(1.5f);
			statisticValues.back().setOutlineThickness(1.5f);
		}

		const sf::Font& menuFont = regularFont;
		const sf::Texture& idle = assets.Textures().Get(Config::Texture::MenuButtonIdle);
		const sf::Texture& selected = assets.Textures().Get(Config::Texture::MenuButtonSelected);

		for (std::size_t index = 0u; index < 3u; index++)
		{
			MenuButton button(menuFont, idle, selected, "", ButtonSize);
			const float totalWidth = ButtonSize.x * 3.f + ButtonGap * 2.f;

			button.SetPosition({ (logicalSize.x - totalWidth) * .5f +
				static_cast<float>(index) * (ButtonSize.x + ButtonGap), 860.f });

			buttonList.Add(std::move(button));
		}

		// Force the real, full-sentence localized content through its first
		// (expensive) layout pass right now, while the level is still starting
		// up, instead of at the dramatic moment the level is actually cleared.
		// An empty/placeholder string would not have primed anything (there are
		// no real glyphs to lay out), so this uses the exact same content Start()
		// will use for a level-complete screen, just with placeholder numbers.
		ApplyContent(Mode::LevelComplete, 1, Statistics{});

		Reset();
	}

	void ResultScreen::Start(Mode newMode, int level, const Statistics& resultStatistics)
	{
		isActive = true;
		isInteractive = false;
		animationElapsedSeconds = 0.f;
		ApplyContent(newMode, level, resultStatistics);

		buttonList.Select(0u, false);
		titleGlow.Invalidate();
		buttonGlow.Invalidate();
		ApplyVisualState();

		audio.PlaySound(Config::Sound::InterfaceActivation, SoundGroup::UI, 80.f, 1.f, SoundPlayback::StopPrevious);
	}

	void ResultScreen::ApplyContent(Mode newMode, int level, const Statistics& resultStatistics)
	{
		mode = newMode;
		statistics = resultStatistics;

		title.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		statisticsTitle.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		statisticsTitle.setString(localization.GetText("results.statistics"));
		TextLayout::CenterText(statisticsTitle, { logicalSize.x * .5f, StatisticsPanelPosition.y + 52.f });

		const sf::Font& regularFont = assets.Fonts().Get(localization.GetRegularFont());
		const sf::Font& bodyFont = assets.Fonts().Get(localization.GetRegularFont(false));

		for (sf::Text& label : statisticLabels)
			label.setFont(bodyFont);

		for (sf::Text& value : statisticValues)
			value.setFont(regularFont);

		for (MenuButton& button : buttonList.GetButtons())
			button.SetFont(regularFont);

		if (mode == Mode::Victory)
		{
			title.setString(localization.GetText("results.victory"));
			buttonList.GetButtons()[0].SetLabel(localization.GetText("results.play_again"));
		}
		else if (mode == Mode::LevelReplay)
		{
			title.setString(localization.FormatText("results.level_complete", "value", std::to_string(level)));
			buttonList.GetButtons()[0].SetLabel(localization.GetText("results.back_levels"));
		}
		else if (mode == Mode::ContentComplete)
		{
			title.setString(localization.FormatText("results.level_complete", "value", std::to_string(level)));
			buttonList.GetButtons()[0].SetLabel(localization.GetText("results.continue"));
		}
		else
		{
			title.setString(localization.FormatText("results.level_complete", "value", std::to_string(level)));
			buttonList.GetButtons()[0].SetLabel(localization.GetText("results.continue"));
		}

		statisticLabels[0].setString(localization.GetText("results.destroyed"));
		statisticLabels[1].setString(
			localization.FormatText("results.armor", "value", std::to_string(statistics.armorPercent)));
		statisticLabels[2].setString(
			localization.FormatText("results.accuracy", "value", std::to_string(statistics.accuracyPercent)) +
			sf::String("  (") + localization.FormatText("results.required", "value",
				std::to_string(statistics.targetAccuracyPercent)) + sf::String(")"));
		statisticLabels[3].setString(
			localization.FormatText("results.parts", "value", std::to_string(statistics.partsCollected)) +
			sf::String(" / ") + sf::String(std::to_string(statistics.partsTotal)));
		statisticLabels[4].setString(localization.GetText("results.level_score"));

		for (std::size_t index = 0u; index < statisticLabels.size(); index++)
		{
			AlignLeft(statisticLabels[index], LabelPositions[index]);
			AlignRight(statisticValues[index], ValuePositions[index]);
		}

		buttonList.GetButtons()[1].SetLabel(localization.GetText("game_over.restart_level"));
		buttonList.GetButtons()[2].SetLabel(localization.GetText("common.back_main"));

		TextLayout::FitWidth(title, TitleFrameSize.x - 140.f, 48u);
		TextLayout::CenterText(title, { logicalSize.x * 0.5f, 190.f });
	}

	void ResultScreen::Reset()
	{
		isActive = false;
		isInteractive = false;
		animationElapsedSeconds = 0.f;

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
		if (!isActive)
			return;

		titleGlow.Update(deltaTime);
		buttonGlow.Update(deltaTime);
		menuCursor.Update(deltaTime);

		animationElapsedSeconds = std::min(InteractiveTime, animationElapsedSeconds + deltaTime);
		isInteractive = animationElapsedSeconds >= InteractiveTime;

		ApplyVisualState();
	}

	std::optional<ResultScreen::Action> ResultScreen::HandleEvent(const sf::Event& event, sf::RenderWindow& window)
	{
		if (!isActive)
			return std::nullopt;

		const auto navigation = gamepad.GetNavigationAction(event);

		if (!isInteractive)
		{
			if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>() ||
				navigation != GamepadManager::NavigationAction::None)
				SkipAnimation();

			return std::nullopt;
		}


		switch (navigation)
		{
			using enum GamepadManager::NavigationAction;

			// The buttons below are laid out in a horizontal row, so navigation
			// between them is Left/Right, not Up/Down.
		case Left:
			buttonList.SelectPrevious();
			return std::nullopt;
		case Right:
			buttonList.SelectNext();
			return std::nullopt;
		case Confirm:
			return ActivateSelected();
		case Back:
			buttonList.Select(2u, false);
			return ActivateSelected();
		}

		if (const auto* key = event.getIf<sf::Event::KeyPressed>())
		{
			if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
				buttonList.SelectPrevious();
			else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
				buttonList.SelectNext();
			else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
				return ActivateSelected();
			else
				return std::nullopt;
		}

		if (const auto* moved = event.getIf<sf::Event::MouseMoved>())
		{
			buttonList.UpdateMouseSelection(window.mapPixelToCoords(moved->position));
			return std::nullopt;
		}
		if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if (pressed->button != sf::Mouse::Button::Left)
				return std::nullopt;

			const sf::Vector2f position = window.mapPixelToCoords(pressed->position);
			for (std::size_t index = 0u; index < buttonList.GetButtons().size(); index++)
			{
				if (buttonList.GetButtons()[index].Contains(position))
				{
					buttonList.Select(index, false);
					return ActivateSelected();
				}
			}
		}

		return std::nullopt;
	}

	void ResultScreen::Draw(sf::RenderTarget& target)
	{
		if (!isActive)
			return;

		target.draw(shade);
		if (Progress(animationElapsedSeconds, TitleStart, TitleDuration) >= 1.f)
		{
			titleGlow.DrawBloom(
				target,
				titleFrame.getGlobalBounds(),
				[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					glowTarget.draw(titleFrame, states);
					glowTarget.draw(title, states);
				},
				SuccessGlowColor,
				false);
		}

		target.draw(titleFrame);
		target.draw(title);
		target.draw(statisticsPanel);
		target.draw(statisticsSeparator);
		target.draw(statisticsTitle);

		for (std::size_t index = 0u; index < statisticLabels.size(); index++)
		{
			if (animationElapsedSeconds < FirstStatisticStart + static_cast<float>(index) * StatisticInterval)
				continue;

			const sf::Text& value = statisticValues[index];

			target.draw(statisticLabels[index]);
			target.draw(value);
		}

		const std::size_t visibleButtons = animationElapsedSeconds >= ThirdButtonStart ? 3u :
			(animationElapsedSeconds >= SecondButtonStart ? 2u : (animationElapsedSeconds >= FirstButtonStart ? 1u : 0u));

		if (isInteractive && visibleButtons > 0u)
		{
			const MenuButton& selected = buttonList.GetButtons()[buttonList.GetSelectedIndex()];

			buttonGlow.DrawBloom(
				target,
				selected.GetBounds(),
				[&selected](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
				{
					selected.Draw(glowTarget, states);
				},
				SelectionGlowColor);
		}

		for (std::size_t index = 0u; index < visibleButtons; index++)
			buttonList.GetButtons()[index].Draw(target);

		if (isInteractive && visibleButtons > 0u)
			buttonGlow.DrawHighlight(
				target, buttonList.GetButtons()[buttonList.GetSelectedIndex()].GetBounds(), SelectionGlowColor);
	}

	void ResultScreen::DrawCursor(sf::RenderWindow& window)
	{
		if (isActive && isInteractive && !gamepad.IsInUse())
			menuCursor.Draw(window);
	}

	bool ResultScreen::IsActive() const noexcept
	{
		return isActive;
	}

	ResultScreen::Mode ResultScreen::GetMode() const noexcept
	{
		return mode;
	}

	void ResultScreen::SkipAnimation()
	{
		animationElapsedSeconds = InteractiveTime;
		isInteractive = true;

		ApplyVisualState();

		buttonList.Select(0u, false);
	}

	void ResultScreen::ApplyVisualState()
	{
		shade.setFillColor(sf::Color(0, 3, 12, ToAlpha(Progress(animationElapsedSeconds, 0.f, DarkenDuration) * 0.76f)));
		const std::uint8_t titleAlpha = ToAlpha(Progress(animationElapsedSeconds, TitleStart, TitleDuration));

		titleFrame.setColor(sf::Color(255, 255, 255, titleAlpha));
		title.setFillColor(sf::Color(225, 255, 230, titleAlpha));
		title.setOutlineColor(sf::Color(0, 92, 30, titleAlpha));

		const std::uint8_t panelAlpha = ToAlpha(Progress(animationElapsedSeconds, StatisticsPanelStart, StatisticsPanelDuration));
		statisticsPanel.setFillColor(sf::Color(2, 13, 27, static_cast<std::uint8_t>(static_cast<float>(panelAlpha) * 0.90f)));
		statisticsPanel.setOutlineColor(sf::Color(25, 205, 240, panelAlpha));
		statisticsSeparator.setFillColor(sf::Color(StatisticsGold.r, StatisticsGold.g, StatisticsGold.b, panelAlpha));
		statisticsTitle.setFillColor(sf::Color(StatisticsGold.r, StatisticsGold.g, StatisticsGold.b, panelAlpha));
		statisticsTitle.setOutlineColor(sf::Color(70, 32, 2, panelAlpha));

		const std::array<int, StatisticLineCount> targetValues =
		{
			statistics.combatScore,
			statistics.armorBonus,
			statistics.accuracyBonus,
			statistics.partsBonus,
			statistics.levelTotal
		};

		for (std::size_t index = 0u; index < statisticLabels.size(); index++)
		{
			const float rowStart = FirstStatisticStart + static_cast<float>(index) * StatisticInterval;
			const std::uint8_t rowAlpha = ToAlpha(Progress(animationElapsedSeconds, rowStart, StatisticFadeDuration));
			const float countProgress = Progress(animationElapsedSeconds, rowStart, ValueCountDuration);
			const float easedProgress = 1.f - std::pow(1.f - countProgress, 3.f);
			const int displayedValue = static_cast<int>(std::lround(static_cast<float>(targetValues[index]) * easedProgress));
			const std::string prefix = index >= 1u && index <= 3u ? "+" : "";

			statisticValues[index].setString(prefix + std::to_string(displayedValue));
			AlignRight(statisticValues[index], ValuePositions[index]);

			const sf::Color labelColor{ index == 4u ? sf::Color(155, 245, 255, rowAlpha) : sf::Color(225, 240, 246, rowAlpha) };

			statisticLabels[index].setFillColor(labelColor);
			statisticLabels[index].setOutlineColor(sf::Color(2, 14, 25, rowAlpha));
			statisticValues[index].setFillColor(sf::Color(215, 252, 255, rowAlpha));
			statisticValues[index].setOutlineColor(sf::Color(5, 95, 125, rowAlpha));
		}
	}

	std::optional<ResultScreen::Action> ResultScreen::ActivateSelected()
	{
		audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
		if (buttonList.GetSelectedIndex() == 0u)
			return Action::Primary;
		if (buttonList.GetSelectedIndex() == 1u)
			return Action::Restart;
		else
			return Action::MainMenu;
	}
}