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
	constexpr sf::Color SelectedTextColor = { 255, 190, 72 };
	constexpr sf::Color DisabledTextColor = { 100, 112, 122 };
	constexpr std::uint8_t DisabledFrameBrightness = 90u;
}

namespace UI
{
	MenuButton::MenuButton(const sf::Font& font, const sf::Texture& idleTexture, const sf::Texture& selectedTexture,
		sf::String labelText, sf::Vector2f buttonSize)
		: idleTexture(idleTexture)
		, selectedTexture(selectedTexture)
		, leftFrame(idleTexture)
		, centerFrame(idleTexture)
		, rightFrame(idleTexture)
		, label(font, std::move(labelText), 38)
		, size(buttonSize)
	{
		const sf::Vector2u textureSize = idleTexture.getSize();
		const int textureWidth = static_cast<int>(textureSize.x);
		const int textureHeight = static_cast<int>(textureSize.y);
		const int capWidth = std::min(textureHeight, textureWidth / 2);
		const int centerWidth = std::max(1, textureWidth - capWidth * 2);

		leftFrame.setTextureRect({ { 0, 0 }, { capWidth, textureHeight } });
		centerFrame.setTextureRect({ { capWidth, 0 }, { centerWidth, textureHeight } });
		rightFrame.setTextureRect({ { textureWidth - capWidth, 0 }, { capWidth, textureHeight } });

		const float uniformScale = size.y / static_cast<float>(textureHeight);
		const float targetCapWidth = static_cast<float>(capWidth) * uniformScale;
		const float targetCenterWidth = std::max(1.f, size.x - targetCapWidth * 2.f);

		leftFrame.setScale({ uniformScale, uniformScale });
		centerFrame.setScale({ targetCenterWidth / static_cast<float>(centerWidth), uniformScale });
		rightFrame.setScale({ uniformScale, uniformScale });

		label.setFillColor(sf::Color::White);

		SetPosition({});
		CenterLabel();
	}

	void MenuButton::SetPosition(sf::Vector2f newPosition)
	{
		position = newPosition;

		const float capWidth = leftFrame.getGlobalBounds().size.x;

		leftFrame.setPosition(newPosition);
		centerFrame.setPosition({ newPosition.x + capWidth, newPosition.y });
		rightFrame.setPosition({
			newPosition.x + size.x - rightFrame.getGlobalBounds().size.x,
			newPosition.y });

		CenterLabel();
	}

	void MenuButton::SetSelected(bool isSelected)
	{
		this->isSelected = isSelected;
		ApplyVisualState();
	}

	void MenuButton::SetEnabled(bool isEnabled)
	{
		this->isEnabled = isEnabled;

		if (!this->isEnabled)
			isSelected = false;

		ApplyVisualState();
	}

	void MenuButton::SetLabel(const sf::String& text)
	{
		label.setCharacterSize(38u);
		label.setString(text);

		CenterLabel();
	}

	void MenuButton::SetFrameOpacity(float opacity)
	{
		frameOpacity = std::clamp(opacity, 0.f, 1.f);
		ApplyVisualState();
	}

	void MenuButton::SetLabelOpacity(float opacity)
	{
		labelOpacity = std::clamp(opacity, 0.f, 1.f);
		ApplyVisualState();
	}

	void MenuButton::SetFont(const sf::Font& font)
	{
		label.setFont(font);
		CenterLabel();
	}

	void MenuButton::SetFrameColor(sf::Color color)
	{
		frameTint = color;
		ApplyVisualState();
	}

	void MenuButton::SetLabelColor(sf::Color color)
	{
		customLabelColor = true;
		labelTint = color;
		ApplyVisualState();
	}

	void MenuButton::SetLabelOutline(sf::Color color, float thickness)
	{
		label.setOutlineColor(color);
		label.setOutlineThickness(thickness);
	}

	bool MenuButton::IsEnabled() const noexcept
	{
		return isEnabled;
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
		// Shrinking by changing characterSize forces a brand new glyph atlas
		// page per size it steps through -- on this SFML/driver combination that
		// first-touch cost is severe (tens of ms per distinct size), so a loop
		// walking down one point at a time could burn the better part of a
		// second on a single label. Measure once at the original size and scale
		// down instead, the same technique TextLayout::FitWidth already uses.
		constexpr unsigned int OriginalSize = 38u;
		constexpr unsigned int MinimumSize = 18u;
		if (label.getCharacterSize() != OriginalSize)
			label.setCharacterSize(OriginalSize);

		label.setScale({ 1.f, 1.f });

		const float maximumWidth = std::max(40.f, size.x - 48.f);
		const float originalWidth = label.getLocalBounds().size.x;

		if (originalWidth > maximumWidth && originalWidth > 0.f)
		{
			const float minimumScale = static_cast<float>(MinimumSize) / static_cast<float>(OriginalSize);
			const float scale = std::clamp(maximumWidth / originalWidth, minimumScale, 1.f);
			label.setScale({ scale, scale });
		}

		const sf::FloatRect bounds = label.getLocalBounds();
		label.setOrigin(
			{
				bounds.position.x + bounds.size.x * 0.5f,
				bounds.position.y + bounds.size.y * 0.5f
			});

		label.setPosition(position + size * 0.5f);
	}

	void MenuButton::ApplyVisualState()
	{
		const auto alpha = static_cast<std::uint8_t>(frameOpacity * 255.f);
		const auto textAlpha = static_cast<std::uint8_t>(labelOpacity * 255.f);

		if (!isEnabled)
		{
			leftFrame.setTexture(idleTexture, false);
			centerFrame.setTexture(idleTexture, false);
			rightFrame.setTexture(idleTexture, false);

			const sf::Color frameColor(DisabledFrameBrightness, DisabledFrameBrightness, DisabledFrameBrightness, alpha);
			leftFrame.setColor(frameColor);
			centerFrame.setColor(frameColor);
			rightFrame.setColor(frameColor);
			label.setFillColor(sf::Color(DisabledTextColor.r, DisabledTextColor.g, DisabledTextColor.b, textAlpha));
			return;
		}

		const sf::Texture& texture = isSelected ? selectedTexture : idleTexture;
		leftFrame.setTexture(texture, false);
		centerFrame.setTexture(texture, false);
		rightFrame.setTexture(texture, false);

		const sf::Color frameColor(frameTint.r, frameTint.g, frameTint.b,
			static_cast<std::uint8_t>(static_cast<unsigned int>(alpha) * frameTint.a / 255u));

		leftFrame.setColor(frameColor);
		centerFrame.setColor(frameColor);
		rightFrame.setColor(frameColor);

		const sf::Color textColor = customLabelColor ? labelTint : (isSelected ? SelectedTextColor : sf::Color::White);

		label.setFillColor(sf::Color(textColor.r, textColor.g, textColor.b, textAlpha));
	}
}