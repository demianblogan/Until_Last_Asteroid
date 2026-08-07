#include "MenuButton.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace
{
    constexpr sf::Color SelectedTextColor{ 255, 190, 72 };
}

MenuButton::MenuButton(
    const sf::Font& font,
    const sf::Texture& idleTexture,
    const sf::Texture& selectedTexture,
    std::string labelText,
    sf::Vector2f buttonSize)
    : idleTexture(idleTexture)
    , selectedTexture(selectedTexture)
    , background(idleTexture)
    , label(font, std::move(labelText), 38)
    , size(buttonSize)
{
    const sf::Vector2u textureSize{ idleTexture.getSize() };
    background.setScale({
        size.x / static_cast<float>(textureSize.x),
        size.y / static_cast<float>(textureSize.y)
    });

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
    background.setTexture(isSelected ? selectedTexture : idleTexture, true);
    label.setFillColor(isSelected ? SelectedTextColor : sf::Color::White);
}

void MenuButton::SetLabel(std::string_view text)
{
    label.setString(std::string(text));
}

void MenuButton::SetFrameOpacity(float opacity)
{
    const auto alpha{ static_cast<std::uint8_t>(std::clamp(opacity, 0.f, 1.f) * 255.f) };
    background.setColor(sf::Color(255, 255, 255, alpha));
}

bool MenuButton::Contains(sf::Vector2f point) const
{
    return background.getGlobalBounds().contains(point);
}

sf::FloatRect MenuButton::GetBounds() const
{
    return background.getGlobalBounds();
}

void MenuButton::Draw(sf::RenderTarget& target) const
{
    Draw(target, sf::RenderStates::Default);
}

void MenuButton::Draw(sf::RenderTarget& target, const sf::RenderStates& states) const
{
    target.draw(background, states);
    target.draw(label, states);
}

void MenuButton::CenterLabel()
{
    const sf::FloatRect bounds{ label.getLocalBounds() };
    label.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });

    label.setPosition(background.getPosition() + size * 0.5f);
}
