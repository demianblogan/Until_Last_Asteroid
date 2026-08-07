#include "MenuButton.h"

#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

namespace
{
    constexpr sf::Color NormalFill{ 10, 24, 38, 220 };
    constexpr sf::Color NormalOutline{ 72, 210, 230 };
    constexpr sf::Color SelectedFill{ 32, 42, 52, 240 };
    constexpr sf::Color SelectedOutline{ 255, 184, 72 };
}

MenuButton::MenuButton(const sf::Font& font, std::string labelText, sf::Vector2f size)
    : background(size)
    , label(font, std::move(labelText), 38)
{
    background.setFillColor(NormalFill);
    background.setOutlineColor(NormalOutline);
    background.setOutlineThickness(2.f);
    label.setFillColor(sf::Color::White);
    CenterLabel();
}

void MenuButton::SetPosition(sf::Vector2f position)
{
    background.setPosition(position);
    CenterLabel();
}

void MenuButton::SetSelected(bool isSelected)
{
    background.setFillColor(isSelected ? SelectedFill : NormalFill);
    background.setOutlineColor(isSelected ? SelectedOutline : NormalOutline);
    label.setFillColor(isSelected ? SelectedOutline : sf::Color::White);
}

bool MenuButton::Contains(sf::Vector2f point) const
{
    return background.getGlobalBounds().contains(point);
}

void MenuButton::Draw(sf::RenderTarget& target) const
{
    target.draw(background);
    target.draw(label);
}

void MenuButton::CenterLabel()
{
    const sf::FloatRect bounds{ label.getLocalBounds() };
    label.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });

    label.setPosition(background.getPosition() + background.getSize() * 0.5f);
}
