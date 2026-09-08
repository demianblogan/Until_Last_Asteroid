#pragma once

#include <cstddef>

#include <SFML/Graphics/Shape.hpp>
#include <SFML/System/Vector2.hpp>

namespace UI
{
	// The only UI widget that inherits from sf::Shape. sf::Shape exists for
	// objects with procedurally computed vertex geometry -- it requires
	// overriding getPointCount()/getPoint() and lets SFML triangulate the
	// outline itself. Rounded corners are exactly that: points computed along
	// an arc, which no built-in SFML shape provides. Every other UI widget
	// (MenuButton, GlowingCursor, NineSliceFrame, ...) is a composite of
	// existing drawables (sf::Sprite, sf::Text, sf::RectangleShape) with no
	// custom geometry of its own, so it has no reason to inherit from
	// sf::Shape -- there would be nothing meaningful to put in getPoint().
	class RoundedRectangleShape final : public sf::Shape
	{
	public:
		RoundedRectangleShape(sf::Vector2f size = {}, float radius = 0.f, std::size_t cornerPointCount = 8u);

		void SetSize(sf::Vector2f size);
		void SetRadius(float radius);

		[[nodiscard]] sf::Vector2f GetSize() const noexcept;
		[[nodiscard]] std::size_t getPointCount() const override;
		[[nodiscard]] sf::Vector2f getPoint(std::size_t index) const override;

	private:
		sf::Vector2f size;
		float radius = 0.f;
		std::size_t cornerPointCount = 8u;
	};
}