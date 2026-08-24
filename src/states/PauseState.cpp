#include "PauseState.h"

#include <algorithm>
#include <string>
#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "gameplay/GameplayLaunch.h"
#include "localization/LocalizationManager.h"
#include "states/StateID.h"
#include "input/GamepadManager.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr sf::Vector2f ButtonSize{ 540.f, 104.f };
    constexpr float ActivationDelay{ 0.12f };
    constexpr float MainMenuFadeOutDuration{ 0.38f };
    constexpr float BlurRadius{ 4.f };
    constexpr sf::Color SelectionGlowColor{ 255, 178, 42 };
    constexpr sf::Color InterfaceGlowColor{ 25, 220, 255 };

    sf::Vector2u EnsureNonZero(sf::Vector2u size)
    {
        return { std::max(1u, size.x), std::max(1u, size.y) };
    }

    sf::Vector2u GetViewportSize(sf::RenderWindow& window)
    {
        const sf::IntRect viewport{ window.getViewport(window.getView()) };
        if (viewport.size.x <= 0 || viewport.size.y <= 0)
            return EnsureNonZero(window.getSize());

        return {
            static_cast<unsigned int>(viewport.size.x),
            static_cast<unsigned int>(viewport.size.y)
        };
    }
}

PauseState::PauseState(StateStack& stateStack, StateContext context)
    : State(stateStack, context)
    , windowSnapshot(EnsureNonZero(context.window.getSize()))
    , horizontalBlur(GetViewportSize(context.window))
    , blurredFrame(GetViewportSize(context.window))
    , blurShader(context.assets.GetShader(Config::Shader::GaussianBlur))
    , darkOverlay(context.logicalSize)
    , titleGlow(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("pause.title"), 92)
    , title(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("pause.title"), 92)
    , neonGlow(context.assets)
    , menuCursor(
        context.assets,
        Config::Texture::MenuPointer,
        { 6.f, 2.f },
        InterfaceGlowColor)
    , screenFade(context.logicalSize)
{
    context.window.setMouseCursorVisible(false);
	const bool tutorialMenu{ context.gameplayLaunch.tutorialRunning };
	const sf::Vector2f firstButtonPosition{ 90.f, tutorialMenu ? 455.f : 570.f };
	const float buttonSpacing{ tutorialMenu ? 108.f : 112.f };
	const float titleY{ tutorialMenu ? 365.f : 480.f };

    windowSnapshot.setSmooth(true);
    horizontalBlur.setSmooth(true);
    blurredFrame.setSmooth(true);

    darkOverlay.setFillColor(sf::Color(1, 8, 19, 158));

    titleGlow.setFillColor(sf::Color(80, 215, 245, 24));
    titleGlow.setOutlineColor(sf::Color(45, 205, 245, 78));
    titleGlow.setOutlineThickness(9.f);
    titleGlow.setLetterSpacing(1.08f);

    title.setFillColor(sf::Color(215, 247, 252));
    title.setOutlineColor(sf::Color(3, 18, 31, 235));
    title.setOutlineThickness(3.5f);
    title.setLetterSpacing(1.08f);

    const sf::FloatRect titleBounds{ title.getLocalBounds() };
    const sf::Vector2f titleOrigin{
        titleBounds.position.x + titleBounds.size.x * 0.5f,
        titleBounds.position.y + titleBounds.size.y * 0.5f
    };
    const sf::Vector2f menuCenter{
		firstButtonPosition.x + ButtonSize.x * 0.5f,
		titleY
    };
    titleGlow.setOrigin(titleOrigin);
    title.setOrigin(titleOrigin);
    titleGlow.setPosition(menuCenter);
    title.setPosition(menuCenter);

    const sf::Font& menuFont{ context.assets.Fonts().Get(context.localization.GetRegularFont()) };
    const sf::Texture& idleTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
    const sf::Texture& selectedTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };

	std::vector<std::pair<sf::String, PauseAction>> menuItems{
		{ context.localization.GetText("pause.resume"), PauseAction::Resume },
		{ context.localization.GetText(tutorialMenu ? "pause.restart_tutorial" : "pause.restart_level"), PauseAction::RestartLevel }
	};
	if (tutorialMenu)
		menuItems.emplace_back(context.localization.GetText("pause.skip_tutorial"), PauseAction::SkipTutorial);
	menuItems.emplace_back(context.localization.GetText("main_menu.options"), PauseAction::Options);
	menuItems.emplace_back(context.localization.GetText("common.back_main"), PauseAction::MainMenu);

    buttons.reserve(menuItems.size());
	buttonActions.reserve(menuItems.size());
	for (std::size_t index{ 0 }; index < menuItems.size(); ++index)
    {
		buttons.emplace_back(menuFont, idleTexture, selectedTexture, "", ButtonSize);
		buttons.back().SetLabel(menuItems[index].first);
        buttons.back().SetPosition(
			firstButtonPosition + sf::Vector2f{ 0.f, buttonSpacing * static_cast<float>(index) });
		buttonActions.push_back(menuItems[index].second);
    }
    Select(0, false);

    localizationRevision = context.localization.GetLanguageRevision();

    musicWasPlaying = context.audio.IsGameplayMusicPlaying();
    context.audio.PauseGameplayMusic();
}

PauseState::~PauseState()
{
    if (musicWasPlaying && !returningToMainMenu)
        GetContext().audio.ResumeGameplayMusic();

    if (GetContext().window.isOpen())
        GetContext().window.setMouseCursorVisible(false);
}

void PauseState::HandleEvent(const sf::Event& event)
{
    if (activationPending || returningToMainMenu)
        return;

    if (GetContext().gamepad.IsPausePressed(event))
    {
        // Pressing the pause button again while already paused closes the
        // menu, the same as Back -- not itself a menu-navigation action.
        BeginActivation(0u);
        return;
    }

    using enum GamepadManager::NavigationAction;
    switch (GetContext().gamepad.GetNavigationAction(event))
    {
    case Up: SelectPrevious(); return;
    case Down: SelectNext(); return;
    case Confirm: BeginActivation(selectedIndex); return;
    case Back: BeginActivation(0u); return;
    default: break;
    }

    if (const auto* mouseMoved{ event.getIf<sf::Event::MouseMoved>() })
    {
        UpdateMouseSelection(mouseMoved->position);
        return;
    }

    if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
    {
        switch (key->code)
        {
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::W:
            SelectPrevious();
            return;

        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::S:
            SelectNext();
            return;

        case sf::Keyboard::Key::Enter:
        case sf::Keyboard::Key::Space:
            BeginActivation(selectedIndex);
            return;

        case sf::Keyboard::Key::Escape:
            BeginActivation(0);
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
        for (std::size_t index{ 0 }; index < buttons.size(); ++index)
        {
            if (buttons[index].Contains(mousePosition))
            {
                Select(index, false);
                BeginActivation(index);
                return;
            }
        }
    }
}

void PauseState::Update(float deltaTime)
{
    // Pause stays on the stack (not popped) while Options is open on top of
    // it, so a language change there wouldn't otherwise be noticed until
    // something else happened to touch this state's text.
    if (localizationRevision != GetContext().localization.GetLanguageRevision())
        RefreshLocalizedContent();

    neonGlow.Update(deltaTime);
    menuCursor.Update(deltaTime);
    screenFade.Update(deltaTime);

    if (returningToMainMenu)
    {
        if (!screenFade.IsActive())
        {
            GetContext().audio.StopGameplayMusic();
            RequestClear();
            RequestPush(StateID::MainMenu);
        }
        return;
    }

    if (!activationPending)
        return;

    activationDelayRemaining -= deltaTime;
    if (activationDelayRemaining > 0.f)
        return;

    activationPending = false;
    CompleteActivation(pendingActivation);
}

void PauseState::Render()
{
    if (!frameCaptured || capturedWindowSize != GetContext().window.getSize())
        CaptureBlurredFrame();

    sf::RenderWindow& window{ GetContext().window };
    sf::Sprite blurredBackground(blurredFrame.getTexture());
    const sf::Vector2u blurredSize{ blurredFrame.getSize() };
    blurredBackground.setScale({
        GetContext().logicalSize.x / static_cast<float>(blurredSize.x),
        GetContext().logicalSize.y / static_cast<float>(blurredSize.y)
    });
    window.draw(blurredBackground);
    window.draw(darkOverlay);
    window.draw(titleGlow);
    window.draw(title);

    if (!buttons.empty())
    {
        const MenuButton& selectedButton{ buttons[selectedIndex] };
        neonGlow.DrawBloom(
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
        neonGlow.DrawHighlight(window, buttons[selectedIndex].GetBounds(), SelectionGlowColor);
}

void PauseState::RenderOverlay()
{
    if (!GetContext().gamepad.IsInUse())
        menuCursor.Draw(GetContext().window);
    screenFade.Draw(GetContext().window);
}

bool PauseState::IsTransparent() const noexcept
{
    return true;
}

void PauseState::CaptureBlurredFrame()
{
    sf::RenderWindow& window{ GetContext().window };
    const sf::Vector2u windowSize{ EnsureNonZero(window.getSize()) };
    if (windowSnapshot.getSize() != windowSize && !windowSnapshot.resize(windowSize))
        return;

    windowSnapshot.update(window);

    sf::IntRect viewport{ window.getViewport(window.getView()) };
    if (viewport.size.x <= 0 || viewport.size.y <= 0)
        viewport = sf::IntRect({ 0, 0 }, sf::Vector2i(windowSize));

    const sf::Vector2u blurSize{
        static_cast<unsigned int>(viewport.size.x),
        static_cast<unsigned int>(viewport.size.y)
    };
    if (horizontalBlur.getSize() != blurSize && !horizontalBlur.resize(blurSize))
        return;
    if (blurredFrame.getSize() != blurSize && !blurredFrame.resize(blurSize))
        return;

    horizontalBlur.setSmooth(true);
    blurredFrame.setSmooth(true);

    sf::Sprite source(windowSnapshot, viewport);

    blurShader.setUniform("source", sf::Shader::CurrentTexture);
    blurShader.setUniform(
        "direction",
        sf::Glsl::Vec2(BlurRadius / GetContext().logicalSize.x, 0.f));

    sf::RenderStates blurStates;
    blurStates.shader = &blurShader;

    horizontalBlur.clear();
    horizontalBlur.draw(source, blurStates);
    horizontalBlur.display();

    sf::Sprite horizontalResult(horizontalBlur.getTexture());
    blurShader.setUniform(
        "direction",
        sf::Glsl::Vec2(0.f, BlurRadius / GetContext().logicalSize.y));

    blurredFrame.clear();
    blurredFrame.draw(horizontalResult, blurStates);
    blurredFrame.display();

    capturedWindowSize = windowSize;
    frameCaptured = true;
}

void PauseState::RefreshLocalizedContent()
{
    const StateContext& context{ GetContext() };
    localizationRevision = context.localization.GetLanguageRevision();
    const bool tutorialMenu{ context.gameplayLaunch.tutorialRunning };

    const sf::Font& headingFont{ context.assets.Fonts().Get(context.localization.GetBoldFont()) };
    titleGlow.setFont(headingFont);
    titleGlow.setString(context.localization.GetText("pause.title"));
    title.setFont(headingFont);
    title.setString(context.localization.GetText("pause.title"));

    const sf::FloatRect titleBounds{ title.getLocalBounds() };
    const sf::Vector2f titleOrigin{
        titleBounds.position.x + titleBounds.size.x * 0.5f,
        titleBounds.position.y + titleBounds.size.y * 0.5f
    };
    titleGlow.setOrigin(titleOrigin);
    title.setOrigin(titleOrigin);

    const sf::Font& menuFont{ context.assets.Fonts().Get(context.localization.GetRegularFont()) };
    std::vector<sf::String> labels{
        context.localization.GetText("pause.resume"),
        context.localization.GetText(tutorialMenu ? "pause.restart_tutorial" : "pause.restart_level")
    };
    if (tutorialMenu)
        labels.push_back(context.localization.GetText("pause.skip_tutorial"));
    labels.push_back(context.localization.GetText("main_menu.options"));
    labels.push_back(context.localization.GetText("common.back_main"));

    for (std::size_t index{ 0u }; index < buttons.size() && index < labels.size(); ++index)
    {
        buttons[index].SetFont(menuFont);
        buttons[index].SetLabel(labels[index]);
    }
    neonGlow.Invalidate();
}

void PauseState::SelectPrevious()
{
    Select(selectedIndex == 0 ? buttons.size() - 1 : selectedIndex - 1);
}

void PauseState::SelectNext()
{
    Select((selectedIndex + 1) % buttons.size());
}

void PauseState::Select(std::size_t index, bool playSound)
{
    const bool selectionChanged{ selectedIndex != index };
    selectedIndex = index;

    for (std::size_t buttonIndex{ 0 }; buttonIndex < buttons.size(); ++buttonIndex)
        buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);

    if (selectionChanged)
        neonGlow.Invalidate();

    if (selectionChanged && playSound)
    {
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect,
            SoundGroup::UI,
            100.f,
            1.f,
            SoundPlayback::StopPrevious);
    }
}

void PauseState::UpdateMouseSelection(sf::Vector2i pixelPosition)
{
    const sf::Vector2f mousePosition{ GetContext().window.mapPixelToCoords(pixelPosition) };
    for (std::size_t index{ 0 }; index < buttons.size(); ++index)
    {
        if (buttons[index].Contains(mousePosition))
        {
            Select(index);
            return;
        }
    }
}

void PauseState::BeginActivation(std::size_t index)
{
    GetContext().audio.PlaySound(
        Config::Sound::ItemPress,
        SoundGroup::UI,
        100.f,
        1.f,
        SoundPlayback::StopPrevious);

    pendingActivation = index;
    activationDelayRemaining = ActivationDelay;
    activationPending = true;
}

void PauseState::CompleteActivation(std::size_t index)
{
	switch (buttonActions.at(index))
    {
    case PauseAction::Resume:
        RequestPop();
        break;

    case PauseAction::RestartLevel:
        GetContext().gameplayLaunch.pendingCommand = GameplayRuntimeCommand::RestartLevel;
        RequestPop();
        break;

    case PauseAction::SkipTutorial:
		GetContext().gameplayLaunch.pendingCommand = GameplayRuntimeCommand::SkipTutorial;
		RequestPop();
		break;

	case PauseAction::Options:
        RequestPush(StateID::PauseOptions);
        break;

    case PauseAction::MainMenu:
        returningToMainMenu = true;
        screenFade.StartFadeOut(MainMenuFadeOutDuration);
        break;
    }
}
