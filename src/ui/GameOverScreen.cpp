#include "GameOverScreen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "input/GamepadManager.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr sf::Vector2f TitleFrameSize = { 1080.f, 180.f };
	constexpr sf::Vector2f ButtonSize = { 700.f, 112.f };
	constexpr float DarkenDuration = 0.45f;
	constexpr float TitleStart = 0.26f;
	constexpr float TitleDuration = 0.28f;
	constexpr float ScoreStart = 0.58f;
	constexpr float ScoreDuration = 0.22f;
	constexpr float FirstButtonStart = 0.82f;
	constexpr float SecondButtonStart = 0.98f;
	constexpr float InteractiveTime = 1.12f;
	constexpr sf::Color FailureGlowColor = { 255, 70, 38 };
	constexpr sf::Color SelectionGlowColor = { 35, 225, 255 };

	float Progress(float animationElapsedSeconds, float start, float duration)
	{
		return std::clamp((animationElapsedSeconds - start) / duration, 0.f, 1.f);
	}

	std::uint8_t ToAlpha(float opacity)
	{
		return static_cast<std::uint8_t>(std::clamp(opacity, 0.f, 1.f) * 255.f);
	}

	std::string FormatDuration(int seconds)
	{
		std::ostringstream stream;
		stream << std::setfill('0') << std::setw(2) << seconds / 60 << ':' << std::setw(2) << seconds % 60;
		return stream.str();
	}
}

GameOverScreen::GameOverScreen(Assets& assets, AudioManager& gameAudio, GamepadManager& gamepadManager,
	LocalizationManager& localizationManager, sf::Vector2f screenSize)
	: assets(assets)
	, audio(gameAudio)
	, gamepad(gamepadManager)
	, localization(localizationManager)
	, logicalSize(screenSize)
	, shade(screenSize)
	, titleFrame(assets.Textures().Get(Config::Texture::GameOverTitleFrame))
	, title(assets.Fonts().Get(localizationManager.GetBoldFont()), localizationManager.GetText("game_over.title"), 104u)
	, finalScore(assets.Fonts().Get(localizationManager.GetRegularFont()), "", 38u)
	, titleGlowEffect(assets)
	, buttonGlowEffect(assets)
	, menuCursor(assets, Config::Texture::MenuPointer, { 6.f, 2.f }, SelectionGlowColor)
{
	const sf::Vector2u textureSize = { titleFrame.getTexture().getSize() };
	titleFrame.setScale({
		TitleFrameSize.x / static_cast<float>(textureSize.x),
		TitleFrameSize.y / static_cast<float>(textureSize.y) });

	titleFrame.setPosition({ (logicalSize.x - TitleFrameSize.x) * 0.5f,	218.f });

	title.setFillColor(sf::Color(255, 145, 92));
	title.setOutlineColor(sf::Color(125, 12, 8));
	title.setOutlineThickness(5.f);
	title.setLetterSpacing(1.08f);

	CenterText(title, { logicalSize.x * 0.5f, 308.f });

	finalScore.setFillColor(sf::Color(225, 245, 250));
	finalScore.setOutlineColor(sf::Color(2, 14, 25, 230));
	finalScore.setOutlineThickness(2.f);

	// Prime with the real localized content (not an empty string -- an
	// empty string has no glyphs to lay out and primes nothing) so the
	// first, expensive layout pass happens now instead of the moment the
	// player actually dies.
	finalScore.setString(localizationManager.FormatText("game_over.final_score", "value", "0"));

	CenterText(finalScore, { logicalSize.x * 0.5f, 448.f });

	const sf::Font& menuFont = assets.Fonts().Get(localizationManager.GetRegularFont());
	const sf::Texture& idleButtonTexture = assets.Textures().Get(Config::Texture::MenuButtonIdle);
	const sf::Texture& selectedButtonTexture = assets.Textures().Get(Config::Texture::MenuButtonSelected);
	const std::array<sf::String, 2> labels =
	{
		localization.GetText("game_over.restart_level"),
		localization.GetText("common.back_main")
	};

	buttons.reserve(labels.size());

	for (std::size_t index = 0u; index < labels.size(); index++)
	{
		buttons.emplace_back(menuFont, idleButtonTexture, selectedButtonTexture, labels[index], ButtonSize);
		buttons.back().SetPosition(
			{
				(logicalSize.x - ButtonSize.x) * 0.5f,
				520.f + static_cast<float>(index) * 132.f
			});
	}

	localizationRevision = localizationManager.GetLanguageRevision();

	Reset();
}

void GameOverScreen::ShowInCampaignMode(int score)
{
	ShowWithSummary(
		localization.FormatText("game_over.final_score", "value", std::to_string(score)),
		localization.GetText("game_over.restart_level"));
}

void GameOverScreen::ShowInHordeMode(int score, int wavesSurvived)
{
	const sf::String summary =
	{
		localization.FormatText("game_over.horde_summary", "value", std::to_string(score)) +
		sf::String("   ") +
		localization.FormatText("game_over.waves", "value", std::to_string(wavesSurvived))
	};

	ShowWithSummary(summary, localization.GetText("game_over.restart_horde"));
}

void GameOverScreen::ShowInRunMode(int survivalSeconds, int recordSeconds)
{
	const sf::String summary =
	{
		localization.FormatText("game_over.time", "value", FormatDuration(survivalSeconds)) +
		sf::String("   ") +
		localization.FormatText("game_over.record", "value", FormatDuration(recordSeconds))
	};

	ShowWithSummary(summary, localization.GetText("game_over.restart_run"));
}

void GameOverScreen::ShowWithSummary(sf::String summary, const sf::String& restartLabel)
{
	if (isBeingShown)
		return;

	isBeingShown = true;
	isInteractive = false;
	animationElapsedSeconds = 0.f;
	selectedButtonIndex = 0u;

	finalScore.setString(std::move(summary));
	CenterText(finalScore, { logicalSize.x * 0.5f, 448.f });

	buttons[0].SetLabel(restartLabel);
	Select(0u, false);

	titleGlowEffect.Invalidate();
	buttonGlowEffect.Invalidate();
	ApplyVisualState();

	audio.PlaySound(Config::Sound::GameOver, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
}

void GameOverScreen::RefreshLocalizedContent()
{
	localizationRevision = localization.GetLanguageRevision();

	title.setFont(assets.Fonts().Get(localization.GetBoldFont()));
	title.setString(localization.GetText("game_over.title"));
	CenterText(title, { logicalSize.x * 0.5f, 308.f });

	const sf::Font& menuFont = assets.Fonts().Get(localization.GetRegularFont());
	finalScore.setFont(menuFont);
	CenterText(finalScore, { logicalSize.x * 0.5f, 448.f });

	buttons[0].SetFont(menuFont);
	buttons[1].SetFont(menuFont);
	buttons[1].SetLabel(localization.GetText("common.back_main"));

	titleGlowEffect.Invalidate();
	buttonGlowEffect.Invalidate();
}

void GameOverScreen::Reset()
{
	isBeingShown = false;
	isInteractive = false;
	animationElapsedSeconds = 0.f;

	shade.setFillColor(sf::Color::Transparent);
	titleFrame.setColor(sf::Color::Transparent);
	title.setFillColor(sf::Color::Transparent);
	title.setOutlineColor(sf::Color::Transparent);
	finalScore.setFillColor(sf::Color::Transparent);
	finalScore.setOutlineColor(sf::Color::Transparent);
}

void GameOverScreen::Update(float deltaTime)
{
	if (localizationRevision != localization.GetLanguageRevision())
		RefreshLocalizedContent();

	if (!isBeingShown)
		return;

	titleGlowEffect.Update(deltaTime);
	buttonGlowEffect.Update(deltaTime);
	menuCursor.Update(deltaTime);

	const bool wasInteractive = isInteractive;
	animationElapsedSeconds = std::min(InteractiveTime, animationElapsedSeconds + deltaTime);
	isInteractive = animationElapsedSeconds >= InteractiveTime; \

		ApplyVisualState();

	if (!wasInteractive && isInteractive)
		Select(0u, false);
}

std::optional<GameOverScreen::Action> GameOverScreen::HandleEvent(const sf::Event& event, sf::RenderWindow& window)
{
	if (!isBeingShown)
		return std::nullopt;

	const GamepadManager::NavigationAction navigation = gamepad.GetNavigationAction(event);

	if (!isInteractive)
	{
		if (event.is<sf::Event::KeyPressed>() ||
			event.is<sf::Event::MouseButtonPressed>() ||
			navigation != GamepadManager::NavigationAction::None)
		{
			SkipAnimation();
		}

		return std::nullopt;
	}

	switch (navigation)
	{
		using enum GamepadManager::NavigationAction;

	case Up:
		SelectPrevious();
		return std::nullopt;

	case Down:
		SelectNext();
		return std::nullopt;

	case Confirm:
		return ActivateSelectedButton();

	case Back:
		Select(1u, false);
		return ActivateSelectedButton();
	}

	if (const auto* key = event.getIf<sf::Event::KeyPressed>())
	{
		if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W)
			SelectPrevious();
		else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S)
			SelectNext();
		else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
			return ActivateSelectedButton();
		else
			return std::nullopt;
	}

	if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>())
	{
		UpdateMouseSelection(window.mapPixelToCoords(mouseMoved->position));
		return std::nullopt;
	}

	if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (mousePressed->button != sf::Mouse::Button::Left)
			return std::nullopt;

		const sf::Vector2f position = window.mapPixelToCoords(mousePressed->position);
		for (std::size_t index = 0u; index < buttons.size(); index++)
		{
			if (buttons[index].Contains(position))
			{
				Select(index, false);
				return ActivateSelectedButton();
			}
		}
	}

	return std::nullopt;
}

void GameOverScreen::Draw(sf::RenderTarget& target)
{
	if (!isBeingShown)
		return;

	target.draw(shade);

	const float titleProgress = Progress(animationElapsedSeconds, TitleStart, TitleDuration);

	if (titleProgress >= 1.f)
	{
		titleGlowEffect.DrawBloom(target, titleFrame.getGlobalBounds(),
			[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				glowTarget.draw(titleFrame, states);
				glowTarget.draw(title, states);
			},
			FailureGlowColor, false);
	}

	target.draw(titleFrame);
	target.draw(title);
	target.draw(finalScore);

	const std::size_t visibleButtons = animationElapsedSeconds >= SecondButtonStart
		? 2u
		: (animationElapsedSeconds >= FirstButtonStart ? 1u : 0u);

	if (isInteractive && visibleButtons > 0u)
	{
		const MenuButton& selectedButton = buttons[selectedButtonIndex];
		buttonGlowEffect.DrawBloom(target, selectedButton.GetBounds(),
			[&selectedButton](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				selectedButton.Draw(glowTarget, states);
			},
			SelectionGlowColor);
	}

	for (std::size_t index = 0u; index < visibleButtons; index++)
		buttons[index].Draw(target);

	if (isInteractive && visibleButtons > 0u)
		buttonGlowEffect.DrawHighlight(target, buttons[selectedButtonIndex].GetBounds(), SelectionGlowColor);
}

void GameOverScreen::DrawCursor(sf::RenderWindow& window)
{
	if (isBeingShown && isInteractive && !gamepad.IsInUse())
		menuCursor.Draw(window);
}

bool GameOverScreen::IsActive() const noexcept
{
	return isBeingShown;
}

void GameOverScreen::SkipAnimation()
{
	animationElapsedSeconds = InteractiveTime;
	isInteractive = true;

	ApplyVisualState();
	Select(0u, false);
}

void GameOverScreen::ApplyVisualState()
{
	const float darken = Progress(animationElapsedSeconds, 0.f, DarkenDuration);
	shade.setFillColor(sf::Color(0, 3, 12, ToAlpha(darken * 0.76f)));

	const std::uint8_t titleAlpha = ToAlpha(Progress(animationElapsedSeconds, TitleStart, TitleDuration));
	titleFrame.setColor(sf::Color(255, 255, 255, titleAlpha));
	title.setFillColor(sf::Color(255, 145, 92, titleAlpha));
	title.setOutlineColor(sf::Color(125, 12, 8, titleAlpha));

	const std::uint8_t scoreAlpha = ToAlpha(Progress(animationElapsedSeconds, ScoreStart, ScoreDuration));
	finalScore.setFillColor(sf::Color(225, 245, 250, scoreAlpha));
	finalScore.setOutlineColor(sf::Color(2, 14, 25, scoreAlpha));
}

void GameOverScreen::Select(std::size_t index, bool needToPlaySound)
{
	const bool isSelectedButtonChanged = selectedButtonIndex != index;
	selectedButtonIndex = index;

	for (std::size_t buttonIndex = 0u; buttonIndex < buttons.size(); buttonIndex++)
		buttons[buttonIndex].SetSelected(buttonIndex == selectedButtonIndex);

	if (isSelectedButtonChanged)
		buttonGlowEffect.Invalidate();

	if (isSelectedButtonChanged && needToPlaySound)
		audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
}

void GameOverScreen::SelectPrevious()
{
	Select(selectedButtonIndex == 0u ? buttons.size() - 1u : selectedButtonIndex - 1u);
}

void GameOverScreen::SelectNext()
{
	Select((selectedButtonIndex + 1u) % buttons.size());
}

void GameOverScreen::UpdateMouseSelection(sf::Vector2f position)
{
	for (std::size_t index = 0u; index < buttons.size(); index++)
	{
		if (buttons[index].Contains(position))
		{
			Select(index);
			return;
		}
	}
}

std::optional<GameOverScreen::Action> GameOverScreen::ActivateSelectedButton()
{
	audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);

	return selectedButtonIndex == 0u ? Action::RestartLevel : Action::MainMenu;
}

void GameOverScreen::CenterText(sf::Text& text, sf::Vector2f position)
{
	const sf::FloatRect bounds = text.getLocalBounds();

	text.setOrigin(
		{
			bounds.position.x + bounds.size.x * 0.5f,
			bounds.position.y + bounds.size.y * 0.5f
		});

	text.setPosition(position);
}