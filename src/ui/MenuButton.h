#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class Font;
	class RenderTarget;
	class Texture;

	struct RenderStates;
}

namespace UI
{
	class MenuButton
	{
	public:
		MenuButton(const sf::Font& font, const sf::Texture& idleTexture, const sf::Texture& selectedTexture,
			sf::String label, sf::Vector2f size);

		void SetPosition(sf::Vector2f position);

		void SetSelected(bool isSelected);
		void SetEnabled(bool isEnabled);

		void SetLabel(const sf::String& text);
		void SetFont(const sf::Font& font);

		void SetFrameOpacity(float opacity);
		void SetLabelOpacity(float opacity);

		void SetFrameColor(sf::Color color);
		void SetLabelColor(sf::Color color);
		void SetLabelOutline(sf::Color color, float thickness);

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

		bool isSelected = false;
		bool isEnabled = true;

		float frameOpacity = 1.f;
		float labelOpacity = 1.f;

		sf::Color frameTint = sf::Color::White;
		bool customLabelColor = false;
		sf::Color labelTint = sf::Color::White;
	};
}