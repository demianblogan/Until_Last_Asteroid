#include "CampaignMenuState.h"

#include <string>
#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "campaign/CampaignSaveManager.h"
#include "gameplay/GameplayLaunch.h"
#include "localization/LocalizationManager.h"
#include "states/StateID.h"
#include "input/gamepad/GamepadManager.h"
#include "ui/MenuTheme.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr sf::Vector2f ButtonSize{ 620.f, 92.f };
	constexpr sf::Vector2f FirstButtonPosition{ 90.f, 300.f };
	constexpr float ButtonSpacing = 105.f;
	constexpr sf::Vector2f DialogButtonSize{ 300.f, 72.f };
	constexpr sf::Vector2f DialogPanelPosition{ 435.f, 320.f };
	constexpr sf::Vector2f DialogPanelSize{ 1050.f, 410.f };
	constexpr float FadeDuration = 0.38f;
}

CampaignMenuState::CampaignMenuState(StateStack& stateStack, StateContext context)
	: MenuState(stateStack, context)
	, buttonGlow(context.assets)
	, titleGlow(context.assets)
	, dialogGlow(context.assets)
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("campaign_menu.title"), 82)
	, statusText(context.assets.Fonts().Get(context.localization.GetRegularFont()), "", 24)
	, buttonList(context.audio, buttonGlow)
	, dialogShade(context.logicalSize)
	, dialogPanel(DialogPanelSize)
	, dialogTitle(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("campaign_menu.new_title"), 42)
	, dialogMessage(context.assets.Fonts().Get(context.localization.GetCurrentLanguage() == Language::English ?
		Config::Font::BodyRegular : context.localization.GetRegularFont()),
		context.localization.GetText("campaign_menu.overwrite_message"),
		27)
{
	context.window.setMouseCursorVisible(false);

	title.setFillColor(sf::Color(215, 247, 252));
	title.setOutlineColor(sf::Color(3, 18, 31, 235));
	title.setOutlineThickness(3.5f);
	title.setLetterSpacing(1.08f);
	UI::TextLayout::CenterText(title, { FirstButtonPosition.x + ButtonSize.x * 0.5f, 195.f });

	statusText.setFillColor(sf::Color(255, 105, 90));
	statusText.setPosition({ FirstButtonPosition.x + 20.f, 950.f });

	const sf::Font& menuFont = context.assets.Fonts().Get(context.localization.GetRegularFont());
	const sf::Texture& idleTexture = context.assets.Textures().Get(Config::Texture::MenuButtonIdle);
	const sf::Texture& selectedTexture = context.assets.Textures().Get(Config::Texture::MenuButtonSelected);

	const auto addButton = [this, &menuFont, &idleTexture, &selectedTexture](
		sf::String label, MenuAction action, bool enabled)
		{
			const std::size_t index = buttonList.GetButtons().size();

			UI::MenuButton button(menuFont, idleTexture, selectedTexture, "", ButtonSize);
			button.SetLabel(label);
			button.SetPosition(FirstButtonPosition + sf::Vector2f{ 0.f, ButtonSpacing * static_cast<float>(index) });
			button.SetEnabled(enabled);

			buttonList.Add(std::move(button));
			buttonActions.push_back(action);
		};

	buttonActions.reserve(6u);

	if (context.campaignSave.HasSave())
	{
		addButton(context.localization.GetText("campaign_menu.continue"), MenuAction::ContinueCampaign, true);
		addButton(context.localization.GetText("campaign_menu.new"), MenuAction::StartNewCampaign, true);
		addButton(context.localization.GetText("campaign_menu.select_level"), MenuAction::SelectLevel, true);
	}
	else
	{
		addButton(context.localization.GetText("campaign_menu.new"), MenuAction::StartNewCampaign, true);
	}
	addButton(context.localization.GetText("campaign_menu.horde"), MenuAction::HordeMode, true);
	addButton(context.localization.GetText("campaign_menu.run"), MenuAction::RunMode, true);
	addButton(context.localization.GetText("common.back_main"), MenuAction::Back, true);
	buttonList.Select(0u, false);

	dialogShade.setFillColor(sf::Color(0, 3, 10, 205));
	dialogPanel.setPosition(DialogPanelPosition);
	dialogPanel.setFillColor(sf::Color(3, 17, 32, 248));
	dialogPanel.setOutlineColor(UI::MenuTheme::InterfaceGlow);
	dialogPanel.setOutlineThickness(2.f);

	dialogTitle.setFillColor(sf::Color(215, 247, 252));
	UI::TextLayout::CenterText(dialogTitle, { 960.f, 410.f });
	dialogMessage.setFillColor(sf::Color(180, 205, 215));
	UI::TextLayout::CenterText(dialogMessage, { 960.f, 495.f });

	dialogButtons.reserve(2u);
	dialogButtons.emplace_back(menuFont, idleTexture, selectedTexture, "", DialogButtonSize);
	dialogButtons.emplace_back(menuFont, idleTexture, selectedTexture, "", DialogButtonSize);
	dialogButtons[0].SetLabel(context.localization.GetText("common.confirm"));
	dialogButtons[1].SetLabel(context.localization.GetText("common.cancel"));
	dialogButtons[0].SetPosition({ 650.f, 585.f });
	dialogButtons[1].SetPosition({ 970.f, 585.f });
	SelectDialogOption(1u, false);

	Chrome().StartFadeIn(0.25f);
}

void CampaignMenuState::HandleEvent(const sf::Event& event)
{
	if (IsTransitioning() || Chrome().IsFading())
		return;

	using enum GamepadManager::NavigationAction;
	const GamepadManager::NavigationAction navigation = GetContext().gamepad.GetNavigationAction(event);

	if (dialogMode != DialogMode::None)
	{
		switch (navigation)
		{
		case Left:
			SelectDialogOption(0u);
			return;
		case Right:
			SelectDialogOption(1u);
			return;
		case Confirm:
			ActivateDialogOption();
			return;
		case Back:
			if (dialogMode == DialogMode::TutorialChoice)
				ChooseTutorial(false);
			else
				CloseDialog();
			return;
		}

		if (const sf::Event::MouseMoved* mouseMoved = event.getIf<sf::Event::MouseMoved>())
		{
			const sf::Vector2f point = GetContext().window.mapPixelToCoords(mouseMoved->position);
			for (std::size_t index = 0u; index < dialogButtons.size(); index++)
				if (dialogButtons[index].Contains(point))
					SelectDialogOption(index);

			return;
		}

		if (const sf::Event::MouseButtonPressed* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
		{
			if (mousePressed->button != sf::Mouse::Button::Left)
				return;

			const sf::Vector2f point = GetContext().window.mapPixelToCoords(mousePressed->position);
			for (std::size_t index = 0u; index < dialogButtons.size(); index++)
			{
				if (dialogButtons[index].Contains(point))
				{
					SelectDialogOption(index, false);
					ActivateDialogOption();
					return;
				}
			}
		}

		if (const sf::Event::KeyPressed* key = event.getIf<sf::Event::KeyPressed>())
		{
			if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
				SelectDialogOption(0u);
			else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
				SelectDialogOption(1u);
			else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
				ActivateDialogOption();
			else if (key->code == sf::Keyboard::Key::Escape)
			{
				if (dialogMode == DialogMode::TutorialChoice)
					ChooseTutorial(false);
				else
					CloseDialog();
			}
		}

		return;
	}

	switch (navigation)
	{
	case Up:
		buttonList.SelectPrevious();
		return;
	case Down:
		buttonList.SelectNext();
		return;
	case Confirm:
		ActivateSelected();
		return;
	case Back:
		RequestPop();
		return;
	}

	if (const sf::Event::MouseMoved* mouseMoved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f point = GetContext().window.mapPixelToCoords(mouseMoved->position);
		Chrome().SetMousePosition(point);
		buttonList.UpdateMouseSelection(point);

		return;
	}

	if (const sf::Event::KeyPressed* key = event.getIf<sf::Event::KeyPressed>())
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
			RequestPop();
			return;
		}
	}

	if (const sf::Event::MouseButtonPressed* mousePressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (mousePressed->button != sf::Mouse::Button::Left)
			return;

		const sf::Vector2f point = GetContext().window.mapPixelToCoords(mousePressed->position);
		for (std::size_t index = 0u; index < buttonList.GetButtons().size(); index++)
		{
			if (buttonList.GetButtons()[index].IsEnabled() && buttonList.GetButtons()[index].Contains(point))
			{
				buttonList.Select(index, false);
				ActivateSelected();
				return;
			}
		}
	}
}

void CampaignMenuState::OnUpdate(float deltaTime)
{
	buttonGlow.Update(deltaTime);
	titleGlow.Update(deltaTime);
	dialogGlow.Update(deltaTime);
}

void CampaignMenuState::OnRender()
{
	sf::RenderWindow& window = GetContext().window;

	titleGlow.DrawBloom(
		window,
		title.getGlobalBounds(),
		[this](sf::RenderTarget& target, const sf::RenderStates& states)
		{
			target.draw(title, states);
		},
		UI::MenuTheme::InterfaceGlow);

	window.draw(title);
	titleGlow.DrawHighlight(window, title.getGlobalBounds(), UI::MenuTheme::InterfaceGlow);

	if (!buttonList.GetButtons().empty())
	{
		const UI::MenuButton& selectedButton = buttonList.GetButtons()[buttonList.GetSelectedIndex()];
		buttonGlow.DrawBloom(
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

	if (!buttonList.GetButtons().empty())
	{
		buttonGlow.DrawHighlight(
			window,
			buttonList.GetButtons()[buttonList.GetSelectedIndex()].GetBounds(),
			UI::MenuTheme::SelectionGlow);
	}

	window.draw(statusText);

	if (dialogMode == DialogMode::None)
		return;

	window.draw(dialogShade);
	window.draw(dialogPanel);
	window.draw(dialogTitle);
	window.draw(dialogMessage);

	const UI::MenuButton& selectedDialogButton = dialogButtons[dialogSelectedIndex];
	dialogGlow.DrawBloom(
		window,
		selectedDialogButton.GetBounds(),
		[&selectedDialogButton](sf::RenderTarget& target, const sf::RenderStates& states)
		{
			selectedDialogButton.Draw(target, states);
		},
		UI::MenuTheme::SelectionGlow);

	for (const UI::MenuButton& button : dialogButtons)
		button.Draw(window);

	dialogGlow.DrawHighlight(window, selectedDialogButton.GetBounds(), UI::MenuTheme::SelectionGlow);
}

void CampaignMenuState::ActivateSelected()
{
	PlayPressSound();

	switch (buttonActions[buttonList.GetSelectedIndex()])
	{
	case MenuAction::ContinueCampaign:
	{
		const CampaignProgress* progress = GetContext().campaignSave.GetProgress();

		if (progress != nullptr && progress->phase == CampaignPhase::AwaitingUpgrades)
		{
			BeginTransition(FadeDuration, [this]
				{
					RequestClear();
					RequestPush(StateID::ShipUpgrades);
				});
		}
		else if (progress != nullptr && progress->phase == CampaignPhase::Finished)
		{
			BeginTransition(FadeDuration, [this]
				{
					RequestPush(StateID::LevelSelect);
					Chrome().StartFadeIn(FadeDuration);
				});
		}
		else
		{
			BeginGameplay(GameplayLaunchMode::ContinueCampaign);
		}
		break;
	}
	case MenuAction::StartNewCampaign:
		if (GetContext().campaignSave.HasSave())
			OpenOverwriteConfirmation();
		else
			StartNewCampaign();
		break;
	case MenuAction::Back:
		RequestPop();
		break;
	case MenuAction::SelectLevel:
		RequestPush(StateID::LevelSelect);
		break;
	case MenuAction::HordeMode:
		BeginGameplay(GameplayLaunchMode::Horde);
		break;
	case MenuAction::RunMode:
		BeginGameplay(GameplayLaunchMode::Run);
		break;
	}
}

void CampaignMenuState::OpenOverwriteConfirmation()
{
	dialogMode = DialogMode::OverwriteCampaign;
	dialogTitle.setString(GetContext().localization.GetText("campaign_menu.new_title"));
	dialogMessage.setString(GetContext().localization.GetText("campaign_menu.overwrite_message"));

	UI::TextLayout::CenterText(dialogTitle, { 960.f, 410.f });
	UI::TextLayout::CenterText(dialogMessage, { 960.f, 495.f });

	dialogButtons[0].SetLabel(GetContext().localization.GetText("common.confirm"));
	dialogButtons[1].SetLabel(GetContext().localization.GetText("common.cancel"));
	dialogGlow.Invalidate();

	SelectDialogOption(1u, false);
}

void CampaignMenuState::OpenTutorialChoice()
{
	dialogMode = DialogMode::TutorialChoice;
	dialogTitle.setString(GetContext().localization.GetText("campaign_menu.tutorial_title"));
	dialogMessage.setString(GetContext().localization.GetText("campaign_menu.tutorial_message"));

	UI::TextLayout::CenterText(dialogTitle, { 960.f, 410.f });
	UI::TextLayout::CenterText(dialogMessage, { 960.f, 495.f });

	dialogButtons[0].SetLabel(GetContext().localization.GetText("common.play"));
	dialogButtons[1].SetLabel(GetContext().localization.GetText("common.skip"));
	dialogGlow.Invalidate();

	SelectDialogOption(0u, false);
}

void CampaignMenuState::CloseDialog()
{
	dialogMode = DialogMode::None;
}

void CampaignMenuState::SelectDialogOption(std::size_t index, bool needToPlaySound)
{
	const bool isChanged = dialogSelectedIndex != index;
	dialogSelectedIndex = index;

	for (std::size_t buttonIndex = 0u; buttonIndex < dialogButtons.size(); buttonIndex++)
		dialogButtons[buttonIndex].SetSelected(buttonIndex == dialogSelectedIndex);

	if (isChanged)
		dialogGlow.Invalidate();

	if (isChanged && needToPlaySound)
		GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
}

void CampaignMenuState::ActivateDialogOption()
{
	PlayPressSound();

	if (dialogMode == DialogMode::OverwriteCampaign)
	{
		if (dialogSelectedIndex == 0u)
			StartNewCampaign();
		else
			CloseDialog();

		return;
	}

	if (dialogMode == DialogMode::TutorialChoice)
		ChooseTutorial(dialogSelectedIndex == 0u);
}

void CampaignMenuState::StartNewCampaign()
{
	dialogMode = DialogMode::None;

	if (!GetContext().campaignSave.StartNewCampaign())
	{
		statusText.setString(GetContext().localization.GetText("campaign_menu.save_error"));
		return;
	}

	OpenTutorialChoice();
}

void CampaignMenuState::ChooseTutorial(bool needToPlayTutorial)
{
	dialogMode = DialogMode::None;

	if (!needToPlayTutorial)
	{
		if (CampaignProgress* progress = GetContext().campaignSave.EditProgress())
		{
			progress->isTutorialCompleted = true;
			progress->isTutorialSkipped = true;

			static_cast<void>(GetContext().campaignSave.Save());
		}
	}

	BeginGameplay(needToPlayTutorial ? GameplayLaunchMode::Tutorial : GameplayLaunchMode::NewCampaign);
}

void CampaignMenuState::BeginGameplay(GameplayLaunchMode mode)
{
	GetContext().gameplayLaunch.mode = mode;
	BeginTransition(FadeDuration, [this]
		{
			RequestClear();
			RequestPush(StateID::Gameplay);
		});
}
