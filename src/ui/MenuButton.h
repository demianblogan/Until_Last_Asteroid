#pragma once

#include <string>
#include <string_view>

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
    class Font;
    struct RenderStates;
    class RenderTarget;
    class Texture;
}

class MenuButton
{
public:
    MenuButton(
        const sf::Font& font,
        const sf::Texture& idleTexture,
        const sf::Texture& selectedTexture,
        std::string label,
        sf::Vector2f size);

    void SetPosition(sf::Vector2f position);
    void SetSelected(bool isSelected);
    void SetEnabled(bool isEnabled);
    void SetLabel(std::string_view text);
    void SetFrameOpacity(float opacity);

    [[nodiscard]] bool IsEnabled() const noexcept;
    [[nodiscard]] bool Contains(sf::Vector2f point) const;
    [[nodiscard]] sf::FloatRect GetBounds() const;
    void Draw(sf::RenderTarget& target) const;
    void Draw(sf::RenderTarget& target, const sf::RenderStates& states) const;

private:
    void CenterLabel();
    void ApplyVisualState();

    const sf::Texture& idleTexture;
    const sf::Texture& selectedTexture;
	sf::Sprite leftFrame;
	sf::Sprite centerFrame;
	sf::Sprite rightFrame;
    sf::Text label;
	sf::Vector2f position;
    sf::Vector2f size;
    bool selected{ false };
    bool enabled{ true };
    float frameOpacity{ 1.f };
};
