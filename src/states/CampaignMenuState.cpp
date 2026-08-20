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

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "campaign/CampaignSaveManager.h"
#include "game/GameplayLaunch.h"
#include "states/StateId.h"
#include "systems/GamepadManager.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr sf::Vector2f ButtonSize{ 620.f, 92.f };
    constexpr sf::Vector2f FirstButtonPosition{ 90.f, 300.f };
    constexpr float ButtonSpacing{ 105.f };
    constexpr sf::Vector2f DialogButtonSize{ 300.f, 72.f };
    constexpr sf::Vector2f DialogPanelPosition{ 435.f, 320.f };
    constexpr sf::Vector2f DialogPanelSize{ 1050.f, 410.f };
    constexpr sf::Color SelectionGlowColor{ 255, 178, 42 };
    constexpr sf::Color InterfaceGlowColor{ 25, 220, 255 };
    constexpr float FadeDuration{ 0.38f };

    void CenterText(sf::Text& text, sf::Vector2f position)
    {
        const sf::FloatRect bounds{ text.getLocalBounds() };
        text.setOrigin({
            bounds.position.x + bounds.size.x * 0.5f,
            bounds.position.y + bounds.size.y * 0.5f
        });
        text.setPosition(position);
    }
}

CampaignMenuState::CampaignMenuState(StateStack& stateStack, StateContext context)
    : State(stateStack, context)
    , background(context.assets, context.logicalSize)
    , buttonGlow(context.assets)
    , titleGlow(context.assets)
    , dialogGlow(context.assets)
    , menuCursor(
        context.assets,
        Config::Texture::MenuPointer,
        { 6.f, 2.f },
        InterfaceGlowColor)
    , screenFade(context.logicalSize)
    , title(context.assets.Fonts().Get(Config::Font::MenuSemibold), "CAMPAIGN", 82)
    , statusText(context.assets.Fonts().Get(Config::Font::MenuRegular), "", 24)
    , dialogShade(context.logicalSize)
    , dialogPanel(DialogPanelSize)
    , dialogTitle(context.assets.Fonts().Get(Config::Font::MenuSemibold), "START NEW CAMPAIGN?", 42)
    , dialogMessage(
        context.assets.Fonts().Get(Config::Font::BodyRegular),
        "Your existing campaign progress will be permanently replaced.",
        27)
{
    context.window.setMouseCursorVisible(false);

    title.setFillColor(sf::Color(215, 247, 252));
    title.setOutlineColor(sf::Color(3, 18, 31, 235));
    title.setOutlineThickness(3.5f);
    title.setLetterSpacing(1.08f);
    CenterText(title, { FirstButtonPosition.x + ButtonSize.x * 0.5f, 195.f });

    statusText.setFillColor(sf::Color(255, 105, 90));
    statusText.setPosition({ FirstButtonPosition.x + 20.f, 950.f });

    const sf::Font& menuFont{ context.assets.Fonts().Get(Config::Font::MenuRegular) };
    const sf::Texture& idleTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
    const sf::Texture& selectedTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };

    const auto addButton{ [this, &menuFont, &idleTexture, &selectedTexture](
        std::string label,
        MenuAction action,
        bool enabled)
    {
        const std::size_t index{ buttons.size() };
        buttons.emplace_back(menuFont, idleTexture, selectedTexture, std::move(label), ButtonSize);
        buttons.back().SetPosition(
            FirstButtonPosition + sf::Vector2f{ 0.f, ButtonSpacing * static_cast<float>(index) });
        buttons.back().SetEnabled(enabled);
        buttonActions.push_back(action);
    } };

    buttons.reserve(6u);
    buttonActions.reserve(6u);
    if (context.campaignSave.HasSave())
    {
        addButton("Continue Campaign", MenuAction::ContinueCampaign, true);
        addButton("Start New Campaign", MenuAction::StartNewCampaign, true);
        addButton("Select Level", MenuAction::SelectLevel, true);
    }
    else
    {
        addButton("Start New Campaign", MenuAction::StartNewCampaign, true);
    }
    addButton("Horde Mode", MenuAction::HordeMode, true);
    addButton("Run Mode", MenuAction::RunMode, true);
    addButton("Back to Main Menu", MenuAction::Back, true);
    Select(0u, false);

    dialogShade.setFillColor(sf::Color(0, 3, 10, 205));
    dialogPanel.setPosition(DialogPanelPosition);
    dialogPanel.setFillColor(sf::Color(3, 17, 32, 248));
    dialogPanel.setOutlineColor(InterfaceGlowColor);
    dialogPanel.setOutlineThickness(2.f);

    dialogTitle.setFillColor(sf::Color(215, 247, 252));
    CenterText(dialogTitle, { 960.f, 410.f });
    dialogMessage.setFillColor(sf::Color(180, 205, 215));
    CenterText(dialogMessage, { 960.f, 495.f });

    dialogButtons.reserve(2u);
    dialogButtons.emplace_back(menuFont, idleTexture, selectedTexture, "Confirm", DialogButtonSize);
    dialogButtons.emplace_back(menuFont, idleTexture, selectedTexture, "Cancel", DialogButtonSize);
    dialogButtons[0].SetPosition({ 650.f, 585.f });
    dialogButtons[1].SetPosition({ 970.f, 585.f });
    SelectDialogOption(1u, false);

    screenFade.StartFadeIn(0.25f);
}

void CampaignMenuState::HandleEvent(const sf::Event& event)
{
    if (launchingGameplay || launchingUpgrades || launchingLevelSelect || screenFade.IsActive())
        return;

    using enum GamepadManager::NavigationAction;
    const auto navigation{ GetContext().gamepad.GetNavigationAction(event) };

    if (dialogMode != DialogMode::None)
    {
        switch (navigation)
        {
        case Left: SelectDialogOption(0u); return;
        case Right: SelectDialogOption(1u); return;
        case Confirm: ActivateDialogOption(); return;
        case Back:
            if (dialogMode == DialogMode::TutorialChoice)
                ChooseTutorial(false);
            else
                CloseDialog();
            return;
        default: break;
        }

        if (const auto* mouseMoved{ event.getIf<sf::Event::MouseMoved>() })
        {
            const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mouseMoved->position) };
            for (std::size_t index{ 0u }; index < dialogButtons.size(); ++index)
            {
                if (dialogButtons[index].Contains(point))
                    SelectDialogOption(index);
            }
            return;
        }

        if (const auto* mousePressed{ event.getIf<sf::Event::MouseButtonPressed>() })
        {
            if (mousePressed->button != sf::Mouse::Button::Left)
                return;
            const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mousePressed->position) };
            for (std::size_t index{ 0u }; index < dialogButtons.size(); ++index)
            {
                if (dialogButtons[index].Contains(point))
                {
                    SelectDialogOption(index, false);
                    ActivateDialogOption();
                    return;
                }
            }
        }

        if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
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
    case Up: SelectPrevious(); return;
    case Down: SelectNext(); return;
    case Confirm: ActivateSelected(); return;
    case Back: RequestPop(); return;
    default: break;
    }

    if (const auto* mouseMoved{ event.getIf<sf::Event::MouseMoved>() })
    {
        const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mouseMoved->position) };
        background.SetMousePosition(point);
        UpdateMouseSelection(mouseMoved->position);
        return;
    }

    if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
    {
        switch (key->code)
        {
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::W: SelectPrevious(); return;
        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::S: SelectNext(); return;
        case sf::Keyboard::Key::Enter:
        case sf::Keyboard::Key::Space: ActivateSelected(); return;
        case sf::Keyboard::Key::Escape: RequestPop(); return;
        default: break;
        }
    }

    if (const auto* mousePressed{ event.getIf<sf::Event::MouseButtonPressed>() })
    {
        if (mousePressed->button != sf::Mouse::Button::Left)
            return;
        const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mousePressed->position) };
        for (std::size_t index{ 0u }; index < buttons.size(); ++index)
        {
            if (buttons[index].IsEnabled() && buttons[index].Contains(point))
            {
                Select(index, false);
                ActivateSelected();
                return;
            }
        }
    }
}

void CampaignMenuState::Update(float deltaTime)
{
    background.Update(deltaTime);
    buttonGlow.Update(deltaTime);
    titleGlow.Update(deltaTime);
    dialogGlow.Update(deltaTime);
    menuCursor.Update(deltaTime);
    screenFade.Update(deltaTime);

    if (launchingGameplay && !screenFade.IsActive())
    {
        RequestClear();
        RequestPush(StateId::Gameplay);
    }
	else if (launchingUpgrades && !screenFade.IsActive())
	{
		RequestClear();
		RequestPush(StateId::ShipUpgrades);
	}
	else if (launchingLevelSelect && !screenFade.IsActive())
	{
		RequestPush(StateId::LevelSelect);
	}
}

void CampaignMenuState::Render()
{
    sf::RenderWindow& window{ GetContext().window };
    background.Draw(window);

    titleGlow.DrawBloom(
        window,
        title.getGlobalBounds(),
        [this](sf::RenderTarget& target, const sf::RenderStates& states)
        {
            target.draw(title, states);
        },
        InterfaceGlowColor);
    window.draw(title);
    titleGlow.DrawHighlight(window, title.getGlobalBounds(), InterfaceGlowColor);

    if (!buttons.empty())
    {
        const MenuButton& selectedButton{ buttons[selectedIndex] };
        buttonGlow.DrawBloom(
            window,
            selectedButton.GetBounds(),
            [&selectedButton](sf::RenderTarget& target, const sf::RenderStates& states)
            {
                selectedButton.Draw(target, states);
            },
            SelectionGlowColor);
    }

    for (const MenuButton& button : buttons)
        button.Draw(window);
    if (!buttons.empty())
        buttonGlow.DrawHighlight(window, buttons[selectedIndex].GetBounds(), SelectionGlowColor);
    window.draw(statusText);

    if (dialogMode == DialogMode::None)
        return;

    window.draw(dialogShade);
    window.draw(dialogPanel);
    window.draw(dialogTitle);
    window.draw(dialogMessage);

    const MenuButton& selectedDialogButton{ dialogButtons[dialogSelectedIndex] };
    dialogGlow.DrawBloom(
        window,
        selectedDialogButton.GetBounds(),
        [&selectedDialogButton](sf::RenderTarget& target, const sf::RenderStates& states)
        {
            selectedDialogButton.Draw(target, states);
        },
        SelectionGlowColor);
    for (const MenuButton& button : dialogButtons)
        button.Draw(window);
    dialogGlow.DrawHighlight(window, selectedDialogButton.GetBounds(), SelectionGlowColor);
}

void CampaignMenuState::RenderOverlay()
{
    if (!GetContext().gamepad.IsUsingGamepad())
        menuCursor.Draw(GetContext().window);
    screenFade.Draw(GetContext().window);
}

void CampaignMenuState::SelectPrevious()
{
    std::size_t index{ selectedIndex };
    do
    {
        index = index == 0u ? buttons.size() - 1u : index - 1u;
    } while (!buttons[index].IsEnabled() && index != selectedIndex);
    Select(index);
}

void CampaignMenuState::SelectNext()
{
    std::size_t index{ selectedIndex };
    do
    {
        index = (index + 1u) % buttons.size();
    } while (!buttons[index].IsEnabled() && index != selectedIndex);
    Select(index);
}

void CampaignMenuState::Select(std::size_t index, bool playSound)
{
    if (index >= buttons.size() || !buttons[index].IsEnabled())
        return;

    const bool changed{ selectedIndex != index };
    selectedIndex = index;
    for (std::size_t buttonIndex{ 0u }; buttonIndex < buttons.size(); ++buttonIndex)
        buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);

    if (changed)
        buttonGlow.Invalidate();
    if (changed && playSound)
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f, SoundPlayback::Restart);
}

void CampaignMenuState::UpdateMouseSelection(sf::Vector2i pixelPosition)
{
    const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pixelPosition) };
    for (std::size_t index{ 0u }; index < buttons.size(); ++index)
    {
        if (buttons[index].IsEnabled() && buttons[index].Contains(point))
        {
            Select(index);
            return;
        }
    }
}

void CampaignMenuState::ActivateSelected()
{
    PlayPressSound();
    switch (buttonActions[selectedIndex])
    {
    case MenuAction::ContinueCampaign:
		if (const CampaignProgress* progress{ GetContext().campaignSave.GetProgress() };
			progress != nullptr && progress->phase == CampaignPhase::AwaitingUpgrades)
		{
			launchingUpgrades = true;
			screenFade.StartFadeOut(FadeDuration);
		}
		else if (progress != nullptr && progress->phase == CampaignPhase::ContentComplete)
		{
			launchingLevelSelect = true;
			screenFade.StartFadeOut(FadeDuration);
		}
		else
			BeginGameplay(GameplayLaunchMode::ContinueCampaign);
        break;
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
		RequestPush(StateId::LevelSelect);
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
    dialogTitle.setString("START NEW CAMPAIGN?");
    dialogMessage.setString("Your existing campaign progress will be permanently replaced.");
    CenterText(dialogTitle, { 960.f, 410.f });
    CenterText(dialogMessage, { 960.f, 495.f });
    dialogButtons[0].SetLabel("Confirm");
    dialogButtons[1].SetLabel("Cancel");
    dialogGlow.Invalidate();
    SelectDialogOption(1u, false);
}

void CampaignMenuState::OpenTutorialChoice()
{
    dialogMode = DialogMode::TutorialChoice;
    dialogTitle.setString("PLAY THE TUTORIAL?");
    dialogMessage.setString(
        "Learn the basics, Parts, and ship upgrades before Level 1.");
    CenterText(dialogTitle, { 960.f, 410.f });
    CenterText(dialogMessage, { 960.f, 495.f });
    dialogButtons[0].SetLabel("Play");
    dialogButtons[1].SetLabel("Skip");
    dialogGlow.Invalidate();
    SelectDialogOption(0u, false);
}

void CampaignMenuState::CloseDialog()
{
    dialogMode = DialogMode::None;
}

void CampaignMenuState::SelectDialogOption(std::size_t index, bool playSound)
{
    const bool changed{ dialogSelectedIndex != index };
    dialogSelectedIndex = index;
    for (std::size_t buttonIndex{ 0u }; buttonIndex < dialogButtons.size(); ++buttonIndex)
        dialogButtons[buttonIndex].SetSelected(buttonIndex == dialogSelectedIndex);
    if (changed)
        dialogGlow.Invalidate();
    if (changed && playSound)
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f, SoundPlayback::Restart);
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
        statusText.setString("Unable to create campaign save. Check access to Local AppData.");
        return;
    }

    OpenTutorialChoice();
}

void CampaignMenuState::ChooseTutorial(bool playTutorial)
{
    dialogMode = DialogMode::None;
    if (!playTutorial)
    {
        if (CampaignProgress* progress{ GetContext().campaignSave.EditProgress() })
        {
            progress->tutorialCompleted = true;
            static_cast<void>(GetContext().campaignSave.Save());
        }
    }
    BeginGameplay(playTutorial
        ? GameplayLaunchMode::Tutorial
        : GameplayLaunchMode::NewCampaign);
}

void CampaignMenuState::BeginGameplay(GameplayLaunchMode mode)
{
    GetContext().gameplayLaunch.mode = mode;
    launchingGameplay = true;
    screenFade.StartFadeOut(FadeDuration);
}

void CampaignMenuState::PlayPressSound()
{
    GetContext().audio.PlaySound(
        Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f, SoundPlayback::Restart);
}
