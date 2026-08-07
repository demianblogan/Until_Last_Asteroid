#include "MainMenuState.h"

#include <string>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"
#include "core/GameVersion.h"
#include "states/StateId.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr sf::Vector2f ButtonSize{ 440.f, 72.f };
    constexpr sf::Vector2f FirstButtonPosition{ 120.f, 650.f };
    constexpr float ButtonSpacing{ 88.f };
}

MainMenuState::MainMenuState(StateStack& stateStack, StateContext context)
    : State(stateStack, context)
    , background(context.logicalSize)
    , title(context.assets.Fonts().Get(Config::Font::GUI), "UNTIL LAST ASTEROID", 94)
    , version(context.assets.Fonts().Get(Config::Font::GUI), std::string(GameVersion::Text), 24)
{
    context.window.setMouseCursorVisible(true);
    context.window.setMouseCursor(context.assets.GetCursor(Config::Cursor::Arrow));

    background.setFillColor(sf::Color(3, 8, 16));

    title.setFillColor(sf::Color(110, 225, 240));
    const sf::FloatRect titleBounds{ title.getLocalBounds() };
    title.setOrigin({
        titleBounds.position.x + titleBounds.size.x * 0.5f,
        titleBounds.position.y + titleBounds.size.y * 0.5f
    });
    title.setPosition({ context.logicalSize.x * 0.5f, 145.f });

    version.setFillColor(sf::Color(145, 160, 175));
    const sf::FloatRect versionBounds{ version.getLocalBounds() };
    version.setOrigin({
        versionBounds.position.x + versionBounds.size.x,
        versionBounds.position.y + versionBounds.size.y
    });
    version.setPosition(context.logicalSize - sf::Vector2f{ 24.f, 20.f });

    const sf::Font& font{ context.assets.Fonts().Get(Config::Font::GUI) };
    buttons.emplace_back(font, "START GAME", ButtonSize);
    buttons.emplace_back(font, "SCORES", ButtonSize);
    buttons.emplace_back(font, "OPTIONS", ButtonSize);
    buttons.emplace_back(font, "QUIT", ButtonSize);

    for (std::size_t index{ 0 }; index < buttons.size(); ++index)
        buttons[index].SetPosition(FirstButtonPosition + sf::Vector2f{ 0.f, ButtonSpacing * static_cast<float>(index) });

    Select(0);

    sf::Music& backgroundMusic{ context.assets.Music().Get(Config::Music::BackgroundTheme) };
    backgroundMusic.setLooping(true);
    if (backgroundMusic.getStatus() != sf::SoundSource::Status::Playing)
        backgroundMusic.play();
}

void MainMenuState::HandleEvent(const sf::Event& event)
{
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

    if (const auto* mouseMoved{ event.getIf<sf::Event::MouseMoved>() })
    {
        UpdateMouseSelection(mouseMoved->position);
        return;
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
                Select(index);
                ActivateSelected();
                return;
            }
        }
    }
}

void MainMenuState::Update(float)
{
}

void MainMenuState::Render()
{
    sf::RenderWindow& window{ GetContext().window };
    window.draw(background);
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

void MainMenuState::Select(std::size_t index)
{
    selectedIndex = index;
    for (std::size_t buttonIndex{ 0 }; buttonIndex < buttons.size(); ++buttonIndex)
        buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);
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
    switch (selectedIndex)
    {
    case 0:
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
