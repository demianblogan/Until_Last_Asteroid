#include "RoundedRectangleShape.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

RoundedRectangleShape::RoundedRectangleShape(
	sf::Vector2f shapeSize,
	float cornerRadius,
	std::size_t pointsPerCorner)
	: size(shapeSize)
	, radius(cornerRadius)
	, cornerPointCount(std::max<std::size_t>(2u, pointsPerCorner))
{
	update();
}

void RoundedRectangleShape::SetSize(sf::Vector2f shapeSize)
{
	size = shapeSize;
	update();
}

void RoundedRectangleShape::SetRadius(float cornerRadius)
{
	radius = cornerRadius;
	update();
}

sf::Vector2f RoundedRectangleShape::GetSize() const noexcept
{
	return size;
}

std::size_t RoundedRectangleShape::getPointCount() const
{
	return cornerPointCount * 4u;
}

// Returns the index-th vertex of the outline. There is no separate code path
// for the straight edges: each corner contributes a cornerPointCount-point arc,
// and SFML connects consecutive getPoint() results with straight lines, so a
// flat edge simply emerges as the line between the last arc point of one
// corner and the first arc point of the next -- both of which lie exactly on
// that edge (see the angle math below).
sf::Vector2f RoundedRectangleShape::getPoint(std::size_t index) const
{
	// The requested radius can't be honored as-is on a small/thin rectangle
	// (adjacent corners would overlap), so it's capped at half the shorter side.
	const float clampedRadius = std::clamp(radius, 0.f, std::min(size.x, size.y) * 0.5f);

	// Points are grouped consecutively per corner (0=top-left, 1=top-right,
	// 2=bottom-right, 3=bottom-left -- matching the `centers` array below), so
	// integer division/modulo splits index back into "which corner" and
	// "which point within that corner's arc".
	const std::size_t corner = index / cornerPointCount;
	const std::size_t pointInCorner = index % cornerPointCount;

	// 0 at the arc's first point, 1 at its last point. Dividing by
	// (cornerPointCount - 1), not cornerPointCount, is what makes progress hit
	// exactly 1.0 on the last point -- required so that point lands precisely
	// on the straight edge, flush with where the next corner's arc begins.
	const float progress = static_cast<float>(pointInCorner) / static_cast<float>(cornerPointCount - 1u);

	// Each corner sweeps a quarter-circle (90 degrees / half of pi radians).
	const float halfPi = std::numbers::pi_v<float> * 0.5f;

	// The starting angle for corner 0 is offset by pi (180 degrees) so its arc
	// begins pointing along the negative x-axis (left) -- i.e. tangent to the
	// rectangle's left edge -- then sweeps 90 degrees to point straight up,
	// tangent to the top edge. Each subsequent corner picks up exactly where
	// the previous one's arc ended (+halfPi per corner), so the four arcs and
	// the implicit straight edges between them trace one continuous outline.
	const float angle = (static_cast<float>(corner) * halfPi) + std::numbers::pi_v<float> +progress * halfPi;

	// The circle center each corner's arc is drawn around: each rectangle
	// corner, pulled inward by clampedRadius along both axes.
	const std::array<sf::Vector2f, 4> centers =
	{
		sf::Vector2f{ clampedRadius, clampedRadius },                   // top-left
		sf::Vector2f{ size.x - clampedRadius, clampedRadius },          // top-right
		sf::Vector2f{ size.x - clampedRadius, size.y - clampedRadius }, // bottom-right
		sf::Vector2f{ clampedRadius, size.y - clampedRadius }           // bottom-left
	};

	// A point on that corner's circle, at the angle computed above.
	return centers[corner] + sf::Vector2f{ std::cos(angle), std::sin(angle) } * clampedRadius;
}