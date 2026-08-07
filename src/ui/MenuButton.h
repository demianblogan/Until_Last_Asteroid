#pragma once

#include <string>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
    class Font;
    class RenderTarget;
}

class MenuButton
{
public:
    MenuButton(const sf::Font& font, std::string label, sf::Vector2f size);

    void SetPosition(sf::Vector2f position);
    void SetSelected(bool isSelected);

    [[nodiscard]] bool Contains(sf::Vector2f point) const;
    void Draw(sf::RenderTarget& target) const;

private:
    void CenterLabel();

    sf::RectangleShape background;
    sf::Text label;
};
