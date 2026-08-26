#pragma once

#include <cstddef>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/String.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class Font;
	class RenderTarget;
	class Text;
}

namespace UI::OptionsWidgets
{
	// Pure geometry/rendering for the labeled-row widgets (slider, toggle,
	// dropdown, dialog button) shared across OptionsState's pages. These take
	// already-resolved values and bounds and know nothing about GameSettings,
	// Action, or which specific option they belong to -- OptionsState decides
	// *what* to draw, these decide *how*.

	inline constexpr std::size_t MaximumVisibleDropdownItems = 6u;

	void DrawSlider(sf::RenderTarget& target, float rowPositionY, float value, const sf::RenderStates& states);

	void DrawToggle(sf::RenderTarget& target, float rowPositionY, bool isOn,
		sf::Text& onText, sf::Text& offText, const sf::RenderStates& states);

	void DrawDropdownBox(sf::RenderTarget& target, const sf::FloatRect& bounds, bool isEnabled, const sf::RenderStates& states);

	void DrawDropdownItem(sf::RenderTarget& target, const sf::FloatRect& bounds,
		sf::Text& label, bool isSelected, const sf::RenderStates& states);

	void DrawDialogButton(sf::RenderTarget& target, const sf::Font& font,
		const sf::FloatRect& bounds, const sf::String& label, bool isSelected,
		const sf::RenderStates& states);

	void DrawCenteredText(sf::RenderTarget& target, const sf::Font& font,
		const sf::String& value, float centerX, float y, unsigned int size, sf::Color color);

	void DrawText(sf::RenderTarget& target, const sf::Font& font,
		const sf::String& value, sf::Vector2f position, unsigned int size, sf::Color color);

	[[nodiscard]] sf::FloatRect GetValueBoxBounds(float rowPositionY);

	[[nodiscard]] sf::FloatRect GetDropdownItemBounds(
		const sf::FloatRect& valueBoxBounds, std::size_t itemCount, std::size_t visibleIndex);

	[[nodiscard]] sf::FloatRect GetDropdownScrollbarBounds(const sf::FloatRect& valueBoxBounds, std::size_t itemCount);

	[[nodiscard]] float SliderValueFromMouseX(float mouseX);
}