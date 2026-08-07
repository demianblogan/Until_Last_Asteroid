#include "MainMenuState.h"

#include <array>
#include <cstdint>
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "core/GameVersion.h"
#include "states/StateId.h"
#include "utils/ConfigEnums.h"

namespace
{
    const std::string MenuTitle{ "Until Last Asteroid" };
    const std::vector<std::string> MenuLabels{ "Start Game", "Scores", "Options", "Quit" };

    constexpr sf::Vector2f ButtonSize{ 540.f, 104.f };
    constexpr sf::Vector2f FirstButtonPosition{ 90.f, 540.f };
    constexpr float ButtonSpacing{ 112.f };
    constexpr float TitleStartY{ 540.f };
    constexpr float TitleEndY{ 125.f };
    constexpr float ActivationDelay{ 0.12f };
    constexpr std::size_t TypingSoundPoolSize{ 4 };
    constexpr std::array<float, TypingSoundPoolSize> TypingPitches{ 0.97f, 1.02f, 0.99f, 1.04f };
}

MainMenuState::MainMenuState(StateStack& stateStack, StateContext context)
    : State(stateStack, context)
    , background(context.assets, context.logicalSize)
    , introAnimation(MenuTitle, MenuLabels)
    , titleGlow(context.assets.Fonts().Get(Config::Font::MenuSemibold), "", 92)
    , title(context.assets.Fonts().Get(Config::Font::MenuSemibold), "", 92)
    , version(context.assets.Fonts().Get(Config::Font::MenuRegular), std::string(GameVersion::Text), 20)
{
    context.window.setMouseCursorVisible(true);
    context.window.setMouseCursor(context.assets.GetCursor(Config::Cursor::MenuPointer));

    titleGlow.setFillColor(sf::Color(80, 215, 245, 24));
    titleGlow.setOutlineColor(sf::Color(45, 205, 245, 78));
    titleGlow.setOutlineThickness(9.f);
    titleGlow.setLetterSpacing(1.08f);

    title.setFillColor(sf::Color(215, 247, 252));
    title.setOutlineColor(sf::Color(3, 18, 31, 235));
    title.setOutlineThickness(3.5f);
    title.setLetterSpacing(1.08f);
    title.setString(MenuTitle);
    const sf::FloatRect fullTitleBounds{ title.getLocalBounds() };
    titleLeftPosition = context.logicalSize.x * 0.5f
        - fullTitleBounds.size.x * 0.5f
        - fullTitleBounds.position.x;
    title.setOrigin({
        0.f,
        fullTitleBounds.position.y + fullTitleBounds.size.y * 0.5f
    });
    titleGlow.setOrigin(title.getOrigin());
    title.setString("");

    version.setFillColor(sf::Color(145, 160, 175, 0));
    const sf::FloatRect versionBounds{ version.getLocalBounds() };
    version.setOrigin({
        versionBounds.position.x + versionBounds.size.x,
        versionBounds.position.y + versionBounds.size.y
    });
    version.setPosition(context.logicalSize - sf::Vector2f{ 24.f, 20.f });

    const sf::Font& menuFont{ context.assets.Fonts().Get(Config::Font::MenuRegular) };
    const sf::Texture& idleTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
    const sf::Texture& selectedTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };

    buttons.reserve(MenuLabels.size());
    for (std::size_t index{ 0 }; index < MenuLabels.size(); ++index)
    {
        buttons.emplace_back(menuFont, idleTexture, selectedTexture, MenuLabels[index], ButtonSize);
        buttons.back().SetPosition(
            FirstButtonPosition + sf::Vector2f{ 0.f, ButtonSpacing * static_cast<float>(index) });
        buttons.back().SetFrameOpacity(0.f);
    }

    ApplyAnimationState();
}

MainMenuState::~MainMenuState()
{
    GetContext().audio.StopMusic(Config::Music::MainMenuBackground);
}

void MainMenuState::HandleEvent(const sf::Event& event)
{
    if (pendingActivation.has_value())
        return;

    if (const auto* mouseMoved{ event.getIf<sf::Event::MouseMoved>() })
    {
        const sf::Vector2f mousePosition{ GetContext().window.mapPixelToCoords(mouseMoved->position) };
        background.SetMousePosition(mousePosition);

        if (introAnimation.IsInteractive())
            UpdateMouseSelection(mouseMoved->position);

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
            SelectPrevious();
            return;

        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::S:
            SelectNext();
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
        for (std::size_t index{ 0 }; index < buttons.size(); ++index)
        {
            if (buttons[index].Contains(mousePosition))
            {
                Select(index, false);
                ActivateSelected();
                return;
            }
        }
    }
}

void MainMenuState::Update(float deltaTime)
{
    background.Update(deltaTime);
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

void MainMenuState::Render()
{
    sf::RenderWindow& window{ GetContext().window };
    background.Draw(window);
    window.draw(titleGlow);
    window.draw(title);

    for (const MenuButton& button : buttons)
        button.Draw(window);

    window.draw(version);
}

void MainMenuState::SelectPrevious()
{
    Select(selectedIndex == 0 ? buttons.size() - 1 : selectedIndex - 1);
}

void MainMenuState::SelectNext()
{
    Select((selectedIndex + 1) % buttons.size());
}

void MainMenuState::Select(std::size_t index, bool playSound)
{
    const bool selectionChanged{ selectedIndex != index };
    selectedIndex = index;
    for (std::size_t buttonIndex{ 0 }; buttonIndex < buttons.size(); ++buttonIndex)
        buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);

    if (selectionChanged && playSound)
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect,
            SoundGroup::UI,
            45.f,
            1.f,
            SoundPlayback::Restart);
}

void MainMenuState::UpdateMouseSelection(sf::Vector2i pixelPosition)
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

void MainMenuState::ActivateSelected()
{
    GetContext().audio.PlaySound(
        Config::Sound::ItemPress,
        SoundGroup::UI,
        60.f,
        1.f,
        SoundPlayback::Restart);

    pendingActivation = selectedIndex;
    activationDelayRemaining = ActivationDelay;
}

void MainMenuState::CompleteActivation(std::size_t index)
{
    switch (index)
    {
    case 0:
        GetContext().audio.StopMusic(Config::Music::MainMenuBackground);
        RequestClear();
        RequestPush(StateId::Gameplay);
        break;

    case 1:
        RequestPush(StateId::Scores);
        break;

    case 2:
        RequestPush(StateId::Options);
        break;

    case 3:
        GetContext().window.close();
        break;
    }
}

void MainMenuState::ApplyAnimationState()
{
    const std::string visibleTitle{ introAnimation.GetVisibleTitle() };
    titleGlow.setString(visibleTitle);
    title.setString(visibleTitle);

    const float titleY{
        TitleStartY + (TitleEndY - TitleStartY) * introAnimation.GetTitleMoveProgress()
    };
    const sf::Vector2f titlePosition{ titleLeftPosition, titleY };
    titleGlow.setPosition(titlePosition);
    title.setPosition(titlePosition);

    const float frameOpacity{ introAnimation.GetFrameOpacity() };
    for (std::size_t index{ 0 }; index < buttons.size(); ++index)
    {
        buttons[index].SetLabel(introAnimation.GetVisibleMenuItem(index));
        buttons[index].SetFrameOpacity(frameOpacity);
    }

    const auto versionAlpha{ static_cast<std::uint8_t>(frameOpacity * 175.f) };
    version.setFillColor(sf::Color(145, 160, 175, versionAlpha));
}

void MainMenuState::HandleAnimationEvents(const MenuIntroAnimation::Events& events)
{
    PlayTypingSounds(events.typedCharacters);

    if (events.activationStarted)
        GetContext().audio.PlaySound(
            Config::Sound::InterfaceActivation,
            SoundGroup::UI,
            58.f,
            1.f,
            SoundPlayback::Restart);

    if (events.becameInteractive)
    {
        Select(0);
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
            34.f,
            TypingPitches[typingSoundIndex % TypingPitches.size()]);
        ++typingSoundIndex;
    }
}

void MainMenuState::StartMenuMusic()
{
    GetContext().audio.PlayMusic(Config::Music::MainMenuBackground);
}
