#include "OptionsWidgets.h"

#include <algorithm>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Text.hpp>

#include "ui/RoundedRectangleShape.h"

namespace UI::OptionsWidgets
{
	namespace
	{
		constexpr sf::Color Cyan{ 105, 225, 242 };
		constexpr sf::Color BrightCyan{ 205, 250, 255 };
		constexpr sf::Color Orange{ 255, 190, 72 };
		constexpr sf::Color Muted{ 128, 148, 164 };
		constexpr sf::Color Disabled{ 76, 88, 101 };

		constexpr float SliderLeft = 1160.f;
		constexpr float SliderWidth = 380.f;
		constexpr float ValueBoxLeft = 1080.f;
		constexpr float ValueBoxWidth = 500.f;
		constexpr float ValueBoxHeight = 58.f;
		constexpr float DropdownItemHeight = 56.f;
	}

	void DrawSlider(sf::RenderTarget& target, float rowPositionY, float value, const sf::RenderStates& states)
	{
		sf::RectangleShape track({ SliderWidth, 8.f });
		track.setPosition({ SliderLeft, rowPositionY + 38.f });
		track.setFillColor(sf::Color(52, 70, 83));
		target.draw(track, states);

		sf::RectangleShape fill({ SliderWidth * value / 100.f, 8.f });
		fill.setPosition(track.getPosition());
		fill.setFillColor(Cyan);
		target.draw(fill, states);

		sf::CircleShape knob(13.f);
		knob.setOrigin({ 13.f, 13.f });
		knob.setPosition({ SliderLeft + SliderWidth * value / 100.f, rowPositionY + 42.f });
		knob.setFillColor(BrightCyan);
		knob.setOutlineColor(sf::Color(40, 210, 245, 90));
		knob.setOutlineThickness(6.f);
		target.draw(knob, states);
	}

	void DrawToggle(sf::RenderTarget& target, float rowPositionY, bool isOn,
		sf::Text& onText, sf::Text& offText, const sf::RenderStates& states)
	{
		const sf::Vector2f position{ 1280.f, rowPositionY + 17.f };
		UI::RoundedRectangleShape shell({ 270.f, 50.f }, 10.f, 8u);
		shell.setPosition(position);
		shell.setFillColor(sf::Color(3, 13, 23, 230));
		shell.setOutlineColor(sf::Color(55, 93, 111));
		shell.setOutlineThickness(1.f);
		target.draw(shell, states);

		UI::RoundedRectangleShape active({ 128.f, 42.f }, 8.f, 8u);
		active.setPosition(position + sf::Vector2f{ isOn ? 4.f : 138.f, 4.f });
		active.setFillColor(sf::Color(18, 132, 157, 150));
		active.setOutlineColor(Cyan);
		active.setOutlineThickness(1.f);
		target.draw(active, states);
		onText.setPosition(position + sf::Vector2f{ 44.f, 10.f });
		onText.setFillColor(isOn ? BrightCyan : Muted);
		target.draw(onText, states);
		offText.setPosition(position + sf::Vector2f{ 178.f, 10.f });
		offText.setFillColor(isOn ? Muted : BrightCyan);
		target.draw(offText, states);
	}

	void DrawDropdownBox(sf::RenderTarget& target, const sf::FloatRect& bounds, bool isEnabled, const sf::RenderStates& states)
	{
		UI::RoundedRectangleShape box(bounds.size, 10.f, 8u);
		box.setPosition(bounds.position);
		box.setFillColor(isEnabled ? sf::Color(2, 15, 27, 242) : sf::Color(10, 14, 20, 225));
		box.setOutlineColor(isEnabled ? sf::Color(60, 126, 151) : sf::Color(45, 51, 59));
		box.setOutlineThickness(1.5f);
		target.draw(box, states);

		sf::RectangleShape divider({ 1.f, bounds.size.y - 12.f });
		divider.setPosition({ bounds.position.x + bounds.size.x - 58.f, bounds.position.y + 6.f });
		divider.setFillColor(isEnabled ? sf::Color(60, 126, 151) : sf::Color(45, 51, 59));
		target.draw(divider, states);

		UI::RoundedRectangleShape arrowButton({ 46.f, 46.f }, 7.f, 6u);
		arrowButton.setPosition({ bounds.position.x + bounds.size.x - 52.f, bounds.position.y + 6.f });
		arrowButton.setFillColor(isEnabled ? sf::Color(10, 55, 73, 235) : sf::Color(22, 27, 33, 220));
		arrowButton.setOutlineColor(isEnabled ? sf::Color(75, 170, 196) : sf::Color(48, 55, 63));
		arrowButton.setOutlineThickness(1.f);
		target.draw(arrowButton, states);

		sf::ConvexShape arrow(3u);
		arrow.setPoint(0u, { 0.f, 0.f });
		arrow.setPoint(1u, { 18.f, 0.f });
		arrow.setPoint(2u, { 9.f, 10.f });
		arrow.setPosition({ bounds.position.x + bounds.size.x - 38.f, bounds.position.y + 25.f });
		arrow.setFillColor(isEnabled ? BrightCyan : Disabled);
		target.draw(arrow, states);
	}

	void DrawDropdownItem(sf::RenderTarget& target, const sf::FloatRect& bounds,
		sf::Text& label, bool isSelected, const sf::RenderStates& states)
	{
		UI::RoundedRectangleShape item(bounds.size, 7.f, 6u);
		item.setPosition(bounds.position);
		item.setFillColor(isSelected ? sf::Color(12, 58, 76, 250) : sf::Color(3, 16, 28, 248));
		item.setOutlineColor(isSelected ? Cyan : sf::Color(50, 78, 94));
		item.setOutlineThickness(1.f);
		target.draw(item, states);

		label.setPosition(bounds.position + sf::Vector2f{ 20.f, 13.f });
		label.setFillColor(isSelected ? Orange : BrightCyan);
		target.draw(label, states);
	}

	void DrawDialogButton(sf::RenderTarget& target, const sf::Font& font,
		const sf::FloatRect& bounds, const sf::String& labelValue, bool isSelected,
		const sf::RenderStates& states)
	{
		UI::RoundedRectangleShape button(bounds.size, 12.f, 8u);
		button.setPosition(bounds.position);
		button.setFillColor(isSelected ? sf::Color(7, 39, 54, 245) : sf::Color(5, 20, 31, 245));
		button.setOutlineColor(isSelected ? BrightCyan : sf::Color(54, 91, 108));
		button.setOutlineThickness(isSelected ? 2.f : 1.f);
		target.draw(button, states);

		sf::Text label(font, labelValue, 25);
		const sf::FloatRect textBounds{ label.getLocalBounds() };
		label.setOrigin({
			textBounds.position.x + textBounds.size.x * 0.5f,
			textBounds.position.y
			});
		label.setPosition({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + 13.f });
		label.setFillColor(isSelected ? Orange : BrightCyan);
		target.draw(label, states);
	}

	void DrawCenteredText(sf::RenderTarget& target, const sf::Font& font,
		const sf::String& value, float centerX, float y, unsigned int size, sf::Color color)
	{
		sf::Text text(font, value, size);
		const sf::FloatRect bounds{ text.getLocalBounds() };
		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y });
		text.setPosition({ centerX, y });
		text.setFillColor(color);
		target.draw(text);
	}

	void DrawText(sf::RenderTarget& target, const sf::Font& font,
		const sf::String& value, sf::Vector2f position, unsigned int size, sf::Color color)
	{
		sf::Text text(font, value, size);
		text.setPosition(position);
		text.setFillColor(color);
		target.draw(text);
	}

	sf::FloatRect GetValueBoxBounds(float rowPositionY)
	{
		return sf::FloatRect({ ValueBoxLeft, rowPositionY + 12.f }, { ValueBoxWidth, ValueBoxHeight });
	}

	sf::FloatRect GetDropdownItemBounds(const sf::FloatRect& valueBoxBounds, std::size_t itemCount, std::size_t visibleIndex)
	{
		const float itemWidth{ ValueBoxWidth - (itemCount > MaximumVisibleDropdownItems ? 24.f : 0.f) };
		return sf::FloatRect(
			{ valueBoxBounds.position.x,
				valueBoxBounds.position.y + valueBoxBounds.size.y + DropdownItemHeight * static_cast<float>(visibleIndex) },
			{ itemWidth, DropdownItemHeight });
	}

	sf::FloatRect GetDropdownScrollbarBounds(const sf::FloatRect& valueBoxBounds, std::size_t itemCount)
	{
		const std::size_t visibleCount = std::min(MaximumVisibleDropdownItems, itemCount);
		return sf::FloatRect(
			{ valueBoxBounds.position.x + valueBoxBounds.size.x - 17.f,
				valueBoxBounds.position.y + valueBoxBounds.size.y + 8.f },
			{ 10.f, DropdownItemHeight * static_cast<float>(visibleCount) - 16.f });
	}

	float SliderValueFromMouseX(float mouseX)
	{
		return std::clamp((mouseX - SliderLeft) / SliderWidth, 0.f, 1.f) * 100.f;
	}
}