#include "PauseState.h"

#include <algorithm>
#include <array>
#include <string>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "states/StateId.h"
#include "utils/ConfigEnums.h"

namespace
{
    const std::array<std::string, 2> MenuLabels{ "Resume", "Back to Main Menu" };

    constexpr sf::Vector2f ButtonSize{ 540.f, 104.f };
    constexpr sf::Vector2f FirstButtonPosition{ 90.f, 720.f };
    constexpr float ButtonSpacing{ 120.f };
    constexpr float TitleY{ 620.f };
    constexpr float ActivationDelay{ 0.12f };
    constexpr float BlurRadius{ 4.f };

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
    , titleGlow(context.assets.Fonts().Get(Config::Font::MenuSemibold), "PAUSED", 92)
    , title(context.assets.Fonts().Get(Config::Font::MenuSemibold), "PAUSED", 92)
{
    context.window.setMouseCursorVisible(true);
    context.window.setMouseCursor(context.assets.GetCursor(Config::Cursor::MenuPointer));

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
        FirstButtonPosition.x + ButtonSize.x * 0.5f,
        TitleY
    };
    titleGlow.setOrigin(titleOrigin);
    title.setOrigin(titleOrigin);
    titleGlow.setPosition(menuCenter);
    title.setPosition(menuCenter);

    const sf::Font& menuFont{ context.assets.Fonts().Get(Config::Font::MenuRegular) };
    const sf::Texture& idleTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
    const sf::Texture& selectedTexture{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };

    buttons.reserve(MenuLabels.size());
    for (std::size_t index{ 0 }; index < MenuLabels.size(); ++index)
    {
        buttons.emplace_back(menuFont, idleTexture, selectedTexture, MenuLabels[index], ButtonSize);
        buttons.back().SetPosition(
            FirstButtonPosition + sf::Vector2f{ 0.f, ButtonSpacing * static_cast<float>(index) });
    }
    Select(0, false);

    musicWasPlaying = context.audio.IsMusicPlaying(Config::Music::GameplayTheme);
    context.audio.PauseMusic(Config::Music::GameplayTheme);
}

PauseState::~PauseState()
{
    if (musicWasPlaying && !returningToMainMenu)
        GetContext().audio.ResumeMusic(Config::Music::GameplayTheme);

    if (GetContext().window.isOpen())
    {
        GetContext().window.setMouseCursor(
            GetContext().assets.GetCursor(Config::Cursor::GameplayCrosshair));
    }
}

void PauseState::HandleEvent(const sf::Event& event)
{
    if (activationPending)
        return;

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
    if (!frameCaptured)
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

    for (const MenuButton& button : buttons)
        button.Draw(window);
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

    frameCaptured = true;
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

    if (selectionChanged && playSound)
    {
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect,
            SoundGroup::UI,
            45.f,
            1.f,
            SoundPlayback::Restart);
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
        60.f,
        1.f,
        SoundPlayback::Restart);

    pendingActivation = index;
    activationDelayRemaining = ActivationDelay;
    activationPending = true;
}

void PauseState::CompleteActivation(std::size_t index)
{
    switch (index)
    {
    case 0:
        RequestPop();
        break;

    case 1:
        returningToMainMenu = true;
        GetContext().audio.StopMusic(Config::Music::GameplayTheme);
        RequestClear();
        RequestPush(StateId::MainMenu);
        break;
    }
}
