#include "MainMenuState.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "core/GameVersion.h"
#include "localization/LocalizationManager.h"
#include "states/StateID.h"
#include "gameplay/VibrationProfiles.h"
#include "input/gamepad/GamepadManager.h"
#include "ui/MenuTheme.h"
#include "utils/ConfigEnums.h"

namespace
{
	// The main menu as data: label + what activating that row does. Keeping the
	// two together means reordering the menu can't silently desync the actions
	// from the labels (which a positional switch on the row index would).
	enum class MenuAction
	{
		StartCampaign,
		Achievements,
		Records,
		Options,
		Credits,
		Quit
	};

	struct MenuEntry
	{
		const char* labelKey;
		MenuAction action;
	};

	constexpr std::array<MenuEntry, 6> MenuEntries{
		MenuEntry{ "main_menu.start_game", MenuAction::StartCampaign },
		MenuEntry{ "main_menu.achievements", MenuAction::Achievements },
		MenuEntry{ "main_menu.records", MenuAction::Records },
		MenuEntry{ "main_menu.options", MenuAction::Options },
		MenuEntry{ "main_menu.credits", MenuAction::Credits },
		MenuEntry{ "main_menu.quit", MenuAction::Quit }
	};

	std::vector<sf::String> GetMenuLabels(const LocalizationManager& localization)
	{
		std::vector<sf::String> labels;
		labels.reserve(MenuEntries.size());
		for (const MenuEntry& entry : MenuEntries)
			labels.push_back(localization.GetText(entry.labelKey));

		return labels;
	}

	constexpr sf::Vector2f ButtonSize{ 540.f, 82.f };
	constexpr sf::Vector2f FirstButtonPosition{ 90.f, 400.f };
	constexpr float ButtonSpacing{ 92.f };
	constexpr float TitleStartY{ 540.f };
	constexpr float TitleEndY{ 125.f };
	constexpr float ActivationDelay{ 0.12f };
	constexpr float MenuFadeInDuration{ 0.45f };
	constexpr std::size_t TypingSoundPoolSize{ 4 };
	constexpr std::array<float, TypingSoundPoolSize> TypingPitches{ 0.97f, 1.02f, 0.99f, 1.04f };
}

MainMenuState::MainMenuState(StateStack& stateStack, StateContext context)
	: MenuState(stateStack, context)
	, neonGlow(context.assets)
	, titleNeonGlow(context.assets)
	, introAnimation(context.localization.GetText("main_menu.title"), GetMenuLabels(context.localization))
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()), "", 92)
	, version(context.assets.Fonts().Get(Config::Font::MenuRegular), std::string(GameVersion::Text), 20)
	, buttonList(context.audio, neonGlow)
{
	context.window.setMouseCursorVisible(false);
	localizationRevision = context.localization.GetLanguageRevision();

	title.setFillColor(sf::Color(215, 247, 252));
	title.setOutlineColor(sf::Color(3, 18, 31, 235));
	title.setOutlineThickness(3.5f);
	title.setLetterSpacing(1.08f);
	title.setString(context.localization.GetText("main_menu.title"));
	const sf::FloatRect fullTitleBounds{ title.getLocalBounds() };
	titleLeftPosition = context.logicalSize.x * 0.5f
		- fullTitleBounds.size.x * 0.5f
		- fullTitleBounds.position.x;
	title.setOrigin({
		0.f,
		fullTitleBounds.position.y + fullTitleBounds.size.y * 0.5f
	});
	title.setString("");

	version.setFillColor(sf::Color(145, 160, 175, 0));
	const sf::FloatRect versionBounds{ version.getLocalBounds() };
	version.setOrigin({
		versionBounds.position.x + versionBounds.size.x,
		versionBounds.position.y + versionBounds.size.y
	});
	version.setPosition(context.logicalSize - sf::Vector2f{ 24.f, 20.f });

	const sf::Font& menuFont{ context.assets.Fonts().Get(
		context.localization.GetCurrentLanguage() == Language::English
		? Config::Font::MenuRegular
		: (context.localization.GetCurrentLanguage() == Language::Arabic
			? Config::Font::ArabicRegular : Config::Font::LocalizedRegular)) };
	const sf::Texture& idleTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
	const sf::Texture& selectedTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };

	const std::vector<sf::String> menuLabels{ GetMenuLabels(context.localization) };
	for (std::size_t index{ 0 }; index < menuLabels.size(); ++index)
	{
		UI::MenuButton button(menuFont, idleTexture, selectedTexture, "", ButtonSize);
		button.SetLabel(menuLabels[index]);
		button.SetPosition(
			FirstButtonPosition + sf::Vector2f{ 0.f, ButtonSpacing * static_cast<float>(index) });
		button.SetFrameOpacity(0.f);
		buttonList.Add(std::move(button));
	}

	const bool shouldPlayIntro{ !context.mainMenuIntroPlayed };
	context.mainMenuIntroPlayed = true;
	if (shouldPlayIntro)
	{
		ApplyAnimationState();
	}
	else
	{
		static_cast<void>(introAnimation.Skip());
		ApplyAnimationState();
		buttonList.Select(0u, false);
		StartMenuMusic();
	}
	Chrome().StartFadeIn(MenuFadeInDuration);
}

MainMenuState::~MainMenuState()
{
	GetContext().audio.StopMusic(Config::Music::MainMenuBackground);
}

void MainMenuState::HandleEvent(const sf::Event& event)
{
	if (pendingActivation.has_value() || Chrome().IsFading())
		return;

	const GamepadManager::NavigationAction navigation{
		GetContext().gamepad.GetNavigationAction(event) };

	if (!introAnimation.IsInteractive() &&
		navigation != GamepadManager::NavigationAction::None)
	{
		HandleAnimationEvents(introAnimation.Skip());
		ApplyAnimationState();
		return;
	}

	if (introAnimation.IsInteractive())
	{
		using enum GamepadManager::NavigationAction;
		switch (navigation)
		{
		case Up: buttonList.SelectPrevious(); return;
		case Down: buttonList.SelectNext(); return;
		case Confirm: ActivateSelected(); return;
		case Back: GetContext().window.close(); return;
		default: break;
		}
	}

	if (const auto* mouseMoved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const sf::Vector2f mousePosition{ GetContext().window.mapPixelToCoords(mouseMoved->position) };
		Chrome().SetMousePosition(mousePosition);

		if (introAnimation.IsInteractive())
			buttonList.UpdateMouseSelection(mousePosition);

		return;
	}

	if (!introAnimation.IsInteractive())
	{
		if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>())
		{
			HandleAnimationEvents(introAnimation.Skip());
			ApplyAnimationState();
		}

		return;
	}

	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		switch (key->code)
		{
		case sf::Keyboard::Key::Up:
		case sf::Keyboard::Key::W:
			buttonList.SelectPrevious();
			return;

		case sf::Keyboard::Key::Down:
		case sf::Keyboard::Key::S:
			buttonList.SelectNext();
			return;

		case sf::Keyboard::Key::Enter:
		case sf::Keyboard::Key::Space:
			ActivateSelected();
			return;

		case sf::Keyboard::Key::Escape:
			GetContext().window.close();
			return;

		default:
			break;
		}
	}

	if (const auto* mousePressed{ event.getIf<sf::Event::MouseButtonPressed>() })
	{
		if (mousePressed->button != sf::Mouse::Button::Left)
			return;

		const sf::Vector2f mousePosition{ GetContext().window.mapPixelToCoords(mousePressed->position) };
		for (std::size_t index{ 0 }; index < buttonList.GetButtons().size(); ++index)
		{
			if (buttonList.GetButtons()[index].Contains(mousePosition))
			{
				buttonList.Select(index, false);
				ActivateSelected();
				return;
			}
		}
	}
}

void MainMenuState::OnUpdate(float deltaTime)
{
	if (localizationRevision != GetContext().localization.GetLanguageRevision())
		RefreshLocalizedLabels();
	neonGlow.Update(deltaTime);
	titleNeonGlow.Update(deltaTime);

	if (Chrome().IsFading())
		return;

	HandleAnimationEvents(introAnimation.Update(deltaTime));
	ApplyAnimationState();

	if (!pendingActivation.has_value())
		return;

	activationDelayRemaining -= deltaTime;
	if (activationDelayRemaining > 0.f)
		return;

	const std::size_t activatedIndex{ pendingActivation.value() };
	pendingActivation.reset();
	CompleteActivation(activatedIndex);
}

void MainMenuState::RefreshLocalizedLabels()
{
	localizationRevision = GetContext().localization.GetLanguageRevision();
	hasRefreshedLocalizedLabels = true;
	const Language language{ GetContext().localization.GetCurrentLanguage() };
	const auto fontID{ language == Language::English ? Config::Font::MenuRegular
		: (language == Language::Arabic ? Config::Font::ArabicRegular
			: Config::Font::LocalizedRegular) };
	const sf::Font& font{ GetContext().assets.Fonts().Get(fontID) };
	const sf::Font& titleFont{ GetContext().assets.Fonts().Get(GetContext().localization.GetBoldFont()) };
	title.setFont(titleFont);
	title.setString(GetContext().localization.GetText("main_menu.title"));
	const sf::FloatRect fullTitleBounds{ title.getLocalBounds() };
	titleLeftPosition = GetContext().logicalSize.x * 0.5f - fullTitleBounds.size.x * 0.5f - fullTitleBounds.position.x;
	title.setOrigin({ 0.f, fullTitleBounds.position.y + fullTitleBounds.size.y * 0.5f });
	const auto labels{ GetMenuLabels(GetContext().localization) };
	for (std::size_t i{}; i < buttonList.GetButtons().size() && i < labels.size(); ++i)
	{
		buttonList.GetButtons()[i].SetFont(font);
		buttonList.GetButtons()[i].SetLabel(labels[i]);
	}
	neonGlow.Invalidate();
}

void MainMenuState::OnRender()
{
	sf::RenderWindow& window{ GetContext().window };

	if (!title.getString().isEmpty())
	{
		const sf::FloatRect titleBounds{ title.getGlobalBounds() };
		titleNeonGlow.DrawBloom(
			window,
			titleBounds,
			[this](sf::RenderTarget& target, const sf::RenderStates& states)
			{
				target.draw(title, states);
			},
			UI::MenuTheme::InterfaceGlow);
	}

	window.draw(title);
	if (!title.getString().isEmpty())
		titleNeonGlow.DrawHighlight(window, title.getGlobalBounds(), UI::MenuTheme::InterfaceGlow);

	if (introAnimation.IsInteractive() && !buttonList.GetButtons().empty())
	{
		const UI::MenuButton& selectedButton{ buttonList.GetButtons()[buttonList.GetSelectedIndex()] };
		neonGlow.DrawBloom(
			window,
			selectedButton.GetBounds(),
			[&selectedButton](sf::RenderTarget& target, const sf::RenderStates& states)
			{
				selectedButton.Draw(target, states);
			},
			UI::MenuTheme::SelectionGlow);
	}

	for (const UI::MenuButton& button : buttonList.GetButtons())
		button.Draw(window);

	if (introAnimation.IsInteractive() && !buttonList.GetButtons().empty())
		neonGlow.DrawHighlight(window, buttonList.GetButtons()[buttonList.GetSelectedIndex()].GetBounds(), UI::MenuTheme::SelectionGlow);

	window.draw(version);
}

void MainMenuState::ActivateSelected()
{
	PlayPressSound();

	pendingActivation = buttonList.GetSelectedIndex();
	activationDelayRemaining = ActivationDelay;
}

void MainMenuState::CompleteActivation(std::size_t index)
{
	if (index >= MenuEntries.size())
		return;

	switch (MenuEntries[index].action)
	{
	case MenuAction::StartCampaign:
		RequestPush(StateID::CampaignMenu);
		break;

	case MenuAction::Achievements:
		RequestPush(StateID::Achievements);
		break;

	case MenuAction::Records:
		RequestPush(StateID::Records);
		break;

	case MenuAction::Options:
		RequestPush(StateID::Options);
		break;

	case MenuAction::Credits:
		RequestPush(StateID::Credits);
		break;

	case MenuAction::Quit:
		GetContext().window.close();
		break;
	}
}

void MainMenuState::ApplyAnimationState()
{
	const sf::String visibleTitle{ hasRefreshedLocalizedLabels
		? GetContext().localization.GetText("main_menu.title") : introAnimation.GetVisibleTitle() };
	if (title.getString() != visibleTitle)
	{
		title.setString(visibleTitle);
		titleNeonGlow.Invalidate();
	}

	const float titleY{
		TitleStartY + (TitleEndY - TitleStartY) * introAnimation.GetTitleMoveProgress()
	};
	const sf::Vector2f titlePosition{ titleLeftPosition, titleY };
	title.setPosition(titlePosition);

	const float frameOpacity{ introAnimation.GetFrameOpacity() };
	for (std::size_t index{ 0 }; index < buttonList.GetButtons().size(); ++index)
	{
		if (!hasRefreshedLocalizedLabels)
			buttonList.GetButtons()[index].SetLabel(introAnimation.GetVisibleMenuItem(index));
		buttonList.GetButtons()[index].SetFrameOpacity(frameOpacity);
	}

	const auto versionAlpha{ static_cast<std::uint8_t>(frameOpacity * 175.f) };
	version.setFillColor(sf::Color(145, 160, 175, versionAlpha));
}

void MainMenuState::HandleAnimationEvents(const UI::MenuIntroAnimation::Events& events)
{
	PlayTypingSounds(events.typedCharacters);

	if (events.typedCharacters > 0)
		VibrationProfiles::Apply(GetContext().gamepadHaptics, VibrationProfiles::Light);

	if (events.hasActivationStarted)
		GetContext().audio.PlaySound(
			Config::Sound::InterfaceActivation,
			SoundGroup::UI,
			100.f,
			1.f,
			SoundPlayback::StopPrevious);

	if (events.hasBecomeInteractive)
	{
		buttonList.Select(0);
		StartMenuMusic();
	}
}

void MainMenuState::PlayTypingSounds(std::size_t count)
{
	for (std::size_t index{ 0 }; index < count; ++index)
	{
		GetContext().audio.PlaySound(
			Config::Sound::CharacterTyping,
			SoundGroup::UI,
			100.f,
			TypingPitches[typingSoundIndex % TypingPitches.size()]);
		++typingSoundIndex;
	}
}

void MainMenuState::StartMenuMusic()
{
	GetContext().audio.PlayMusic(Config::Music::MainMenuBackground);
}
