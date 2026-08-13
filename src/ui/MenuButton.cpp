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
    constexpr sf::Color DisabledTextColor{ 100, 112, 122 };
    constexpr std::uint8_t DisabledFrameBrightness{ 90u };
}

MenuButton::MenuButton(
    const sf::Font& font,
    const sf::Texture& idleTexture,
    const sf::Texture& selectedTexture,
    std::string labelText,
    sf::Vector2f buttonSize)
    : idleTexture(idleTexture)
    , selectedTexture(selectedTexture)
	, leftFrame(idleTexture)
	, centerFrame(idleTexture)
	, rightFrame(idleTexture)
    , label(font, std::move(labelText), 38)
    , size(buttonSize)
{
    const sf::Vector2u textureSize{ idleTexture.getSize() };
	const int textureWidth{ static_cast<int>(textureSize.x) };
	const int textureHeight{ static_cast<int>(textureSize.y) };
	const int capWidth{ std::min(textureHeight, textureWidth / 2) };
	const int centerWidth{ std::max(1, textureWidth - capWidth * 2) };
	leftFrame.setTextureRect({ { 0, 0 }, { capWidth, textureHeight } });
	centerFrame.setTextureRect({ { capWidth, 0 }, { centerWidth, textureHeight } });
	rightFrame.setTextureRect({
		{ textureWidth - capWidth, 0 }, { capWidth, textureHeight } });
	const float uniformScale{ size.y / static_cast<float>(textureHeight) };
	const float targetCapWidth{ static_cast<float>(capWidth) * uniformScale };
	const float targetCenterWidth{ std::max(1.f, size.x - targetCapWidth * 2.f) };
	leftFrame.setScale({ uniformScale, uniformScale });
	centerFrame.setScale({
		targetCenterWidth / static_cast<float>(centerWidth), uniformScale });
	rightFrame.setScale({ uniformScale, uniformScale });

    label.setFillColor(sf::Color::White);
	SetPosition({});
    CenterLabel();
}

void MenuButton::SetPosition(sf::Vector2f position)
{
	this->position = position;
	const float capWidth{ leftFrame.getGlobalBounds().size.x };
	leftFrame.setPosition(position);
	centerFrame.setPosition({ position.x + capWidth, position.y });
	rightFrame.setPosition({
		position.x + size.x - rightFrame.getGlobalBounds().size.x,
		position.y });
    CenterLabel();
}

void MenuButton::SetSelected(bool isSelected)
{
    selected = isSelected;
    ApplyVisualState();
}

void MenuButton::SetEnabled(bool isEnabled)
{
    enabled = isEnabled;
    if (!enabled)
        selected = false;
    ApplyVisualState();
}

void MenuButton::SetLabel(std::string_view text)
{
    label.setString(std::string(text));
    CenterLabel();
}

void MenuButton::SetFrameOpacity(float opacity)
{
    frameOpacity = std::clamp(opacity, 0.f, 1.f);
    ApplyVisualState();
}

bool MenuButton::IsEnabled() const noexcept
{
    return enabled;
}

bool MenuButton::Contains(sf::Vector2f point) const
{
	return sf::FloatRect(position, size).contains(point);
}

sf::FloatRect MenuButton::GetBounds() const
{
	return { position, size };
}

void MenuButton::Draw(sf::RenderTarget& target) const
{
    Draw(target, sf::RenderStates::Default);
}

void MenuButton::Draw(sf::RenderTarget& target, const sf::RenderStates& states) const
{
	target.draw(leftFrame, states);
	target.draw(centerFrame, states);
	target.draw(rightFrame, states);
    target.draw(label, states);
}

void MenuButton::CenterLabel()
{
    const sf::FloatRect bounds{ label.getLocalBounds() };
    label.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f
    });

	label.setPosition(position + size * 0.5f);
}

void MenuButton::ApplyVisualState()
{
    const auto alpha{ static_cast<std::uint8_t>(frameOpacity * 255.f) };
    if (!enabled)
    {
		leftFrame.setTexture(idleTexture, false);
		centerFrame.setTexture(idleTexture, false);
		rightFrame.setTexture(idleTexture, false);
		const sf::Color frameColor(
            DisabledFrameBrightness,
            DisabledFrameBrightness,
            DisabledFrameBrightness,
			alpha);
		leftFrame.setColor(frameColor);
		centerFrame.setColor(frameColor);
		rightFrame.setColor(frameColor);
        label.setFillColor(sf::Color(
            DisabledTextColor.r,
            DisabledTextColor.g,
            DisabledTextColor.b,
            alpha));
        return;
    }

	const sf::Texture& texture{ selected ? selectedTexture : idleTexture };
	leftFrame.setTexture(texture, false);
	centerFrame.setTexture(texture, false);
	rightFrame.setTexture(texture, false);
	const sf::Color frameColor(255, 255, 255, alpha);
	leftFrame.setColor(frameColor);
	centerFrame.setColor(frameColor);
	rightFrame.setColor(frameColor);
    const sf::Color textColor{ selected ? SelectedTextColor : sf::Color::White };
    label.setFillColor(sf::Color(textColor.r, textColor.g, textColor.b, alpha));
}
