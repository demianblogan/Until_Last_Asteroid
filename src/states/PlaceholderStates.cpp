#include "PlaceholderStates.h"

#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"

PlaceholderState::PlaceholderState(StateStack& stateStack, StateContext context, std::string titleText)
    : State(stateStack, context)
    , background(context.logicalSize)
    , title(context.assets.Fonts().Get(Config::Font::GUI), std::move(titleText), 90)
    , message(context.assets.Fonts().Get(Config::Font::GUI), "COMING IN A FUTURE UPDATE", 38)
    , backButton(context.assets.Fonts().Get(Config::Font::GUI), "BACK", { 360.f, 72.f })
{
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

    backButton.SetPosition({ 120.f, context.logicalSize.y - 150.f });
    backButton.SetSelected(true);
}

void PlaceholderState::HandleEvent(const sf::Event& event)
{
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

void PlaceholderState::Update(float)
{
}

void PlaceholderState::Render()
{
    sf::RenderWindow& window{ GetContext().window };
    window.draw(background);
    window.draw(title);
    window.draw(message);
    backButton.Draw(window);
}

void PlaceholderState::GoBack()
{
    RequestPop();
}

ScoresState::ScoresState(StateStack& stateStack, StateContext context)
    : PlaceholderState(stateStack, context, "SCORES")
{
}

OptionsState::OptionsState(StateStack& stateStack, StateContext context)
    : PlaceholderState(stateStack, context, "OPTIONS")
{
}
