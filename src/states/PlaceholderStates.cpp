#include "PlaceholderStates.h"

#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"

namespace
{
    constexpr float BackDelay{ 0.12f };
    constexpr sf::Color SelectionGlowColor{ 255, 178, 42 };
}

PlaceholderState::PlaceholderState(StateStack& stateStack, StateContext context, std::string titleText)
    : State(stateStack, context)
    , background(context.logicalSize)
    , title(context.assets.Fonts().Get(Config::Font::MenuSemibold), std::move(titleText), 82)
    , message(context.assets.Fonts().Get(Config::Font::MenuRegular), "Coming in a future update", 32)
    , backButton(
        context.assets.Fonts().Get(Config::Font::MenuRegular),
        context.assets.Textures().Get(Config::Texture::MenuButtonIdle),
        context.assets.Textures().Get(Config::Texture::MenuButtonSelected),
        "Back",
        { 420.f, 82.f })
    , neonGlow(context.assets)
{
    context.window.setMouseCursorVisible(true);
    context.window.setMouseCursor(context.assets.GetCursor(Config::Cursor::MenuPointer));

    background.setFillColor(sf::Color(3, 8, 16));

    title.setFillColor(sf::Color(110, 225, 240));
    const sf::FloatRect titleBounds{ title.getLocalBounds() };
    title.setOrigin({
        titleBounds.position.x + titleBounds.size.x * 0.5f,
        titleBounds.position.y + titleBounds.size.y * 0.5f
    });
    title.setPosition({ context.logicalSize.x * 0.5f, 220.f });

    message.setFillColor(sf::Color(145, 160, 175));
    const sf::FloatRect messageBounds{ message.getLocalBounds() };
    message.setOrigin({
        messageBounds.position.x + messageBounds.size.x * 0.5f,
        messageBounds.position.y + messageBounds.size.y * 0.5f
    });
    message.setPosition(context.logicalSize * 0.5f);

    backButton.SetPosition({ 90.f, context.logicalSize.y - 140.f });
    backButton.SetSelected(true);
}

PlaceholderState::~PlaceholderState()
{
    if (GetContext().window.isOpen())
        GetContext().window.setMouseCursorVisible(false);
}

void PlaceholderState::HandleEvent(const sf::Event& event)
{
    if (backRequested)
        return;

    if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
    {
        if (key->code == sf::Keyboard::Key::Escape ||
            key->code == sf::Keyboard::Key::Backspace ||
            key->code == sf::Keyboard::Key::Enter ||
            key->code == sf::Keyboard::Key::Space)
        {
            GoBack();
            return;
        }
    }

    if (const auto* mousePressed{ event.getIf<sf::Event::MouseButtonPressed>() })
    {
        const sf::Vector2f mousePosition{ GetContext().window.mapPixelToCoords(mousePressed->position) };
        if (mousePressed->button == sf::Mouse::Button::Left && backButton.Contains(mousePosition))
            GoBack();
    }
}

void PlaceholderState::Update(float deltaTime)
{
    neonGlow.Update(deltaTime);

    if (!backRequested)
        return;

    backDelayRemaining -= deltaTime;
    if (backDelayRemaining <= 0.f)
        RequestPop();
}

void PlaceholderState::Render()
{
    sf::RenderWindow& window{ GetContext().window };
    window.draw(background);
    window.draw(title);
    window.draw(message);

    neonGlow.DrawBloom(
        window,
        backButton.GetBounds(),
        [this](sf::RenderTarget& target, const sf::RenderStates& states)
        {
            backButton.Draw(target, states);
        },
        SelectionGlowColor);

    backButton.Draw(window);
    neonGlow.DrawHighlight(window, backButton.GetBounds(), SelectionGlowColor);
}

void PlaceholderState::GoBack()
{
    GetContext().audio.PlaySound(
        Config::Sound::ItemPress,
        SoundGroup::UI,
        100.f,
        1.f,
        SoundPlayback::Restart);
    backRequested = true;
    backDelayRemaining = BackDelay;
}

ScoresState::ScoresState(StateStack& stateStack, StateContext context)
    : PlaceholderState(stateStack, context, "SCORES")
{
}
