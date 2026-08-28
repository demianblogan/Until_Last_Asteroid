#include "EnergyShield.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/VertexArray.hpp>

namespace
{
	// The hex-grid line positions only depend on the shield radius, never on
	// its moving center, current color, or pulse -- but were recomputed from
	// scratch (including a cos/sin pair per hexagon corner, up to a few
	// hundred hexagons for a large shield) every single call. Caching the
	// local (center-relative) positions per radius turns that into a cheap
	// per-vertex translate+recolor each frame instead.
	const std::vector<sf::Vector2f>& GetHexGridLocalVertices(float radius)
	{
		static std::unordered_map<int, std::vector<sf::Vector2f>> cache;

		const int key = static_cast<int>(std::lround(radius * 4.f));
		const auto found = cache.find(key);

		if (found != cache.end())
			return found->second;

		// HexRadius is the size of a single hexagon (center-to-corner).
		// HorizontalSpacing/VerticalSpacing is the distance between neighboring
		// hex centers; odd rows are offset by half of HorizontalSpacing (the
		// "+ 8.f" below) so hexagons interlock in the usual brick-like pattern
		// instead of lining up in a plain rectangular grid.
		constexpr float HexRadius = 9.f;
		constexpr float HorizontalSpacing = 16.f;
		constexpr float VerticalSpacing = 14.f;

		// How many rows/columns of hexagons are needed, in each direction from
		// the center, to cover a circle of this radius -- the loop below walks
		// a rectangular block this big and then discards whatever falls
		// outside the circle.
		const int MaximumRow = static_cast<int>(std::ceil(radius / VerticalSpacing));
		const int MaximumColumn = static_cast<int>(std::ceil(radius / HorizontalSpacing));

		std::vector<sf::Vector2f> vertices;
		for (int row = -MaximumRow; row <= MaximumRow; row++)
		{
			for (int column = -MaximumColumn; column <= MaximumColumn; ++column)
			{
				// Candidate hex center for this row/column, in local space
				// (shield center = origin). Odd rows are staggered sideways.
				const float x = column * HorizontalSpacing + (row % 2 == 0 ? 0.f : 8.f);
				const float y = row * VerticalSpacing;

				// Skip hexagons whose center falls outside the shield's circle
				// (with a small margin) -- this is what turns the rectangular
				// row/column block above into a round hex-grid patch.
				if (x * x + y * y > (radius - HexRadius - 3.f) * (radius - HexRadius - 3.f))
				{
					continue;
				}

				// The 6 corners of this hexagon, computed from its center via
				// the standard "point at angle, HexRadius away" formula.
				// Starting at 30 deg (Pi/6) and stepping by 60 deg (Pi/3) gives
				// a flat-top hexagon oriented to match the row/column layout above.
				std::array<sf::Vector2f, 6> corners;

				for (std::size_t corner = 0u; corner < corners.size(); corner++)
				{
					const float Pi = std::numbers::pi_v<float>;
					const float Angle = Pi / 6.f + static_cast<float>(corner) * Pi / 3.f;

					corners[corner] = sf::Vector2f{ x, y } + sf::Vector2f{
						std::cos(Angle) * HexRadius,
						std::sin(Angle) * HexRadius };
				}

				// DrawEnergyShield renders this as sf::PrimitiveType::Lines, which
				// treats every consecutive PAIR of vertices as one independent line
				// segment -- not as a connected outline. So each hexagon contributes
				// its 6 edges as 6 separate (corner, nextCorner) vertex pairs here,
				// rather than just its 6 corners.
				for (std::size_t corner = 0u; corner < corners.size(); corner++)
				{
					vertices.push_back(corners[corner]);
					vertices.push_back(corners[(corner + 1u) % corners.size()]);
				}
			}
		}

		// Cache this radius's hex grid (in local space) so the next call with
		// the same (quantized) radius skips straight to the lookup above --
		// DrawEnergyShield only needs to translate these to the shield's
		// current world position every frame, not regenerate them.
		return cache.emplace(key, std::move(vertices)).first->second;
	}
}

void Rendering::DrawEnergyShield(sf::RenderTarget& target, sf::Vector2f center, float radius, float pulse,
	sf::Color shellColor, sf::Color outlineColor, sf::Color glowColor, float hexOpacity, sf::RenderStates states)
{
	// 1) The shield's main body: one big, mostly-transparent filled circle
	// with a slightly-less-transparent outline. Both alphas are scaled by
	// `pulse` so the whole shell breathes in and out over time.
	constexpr std::size_t ShieldCirclePoints = 64u;
	sf::CircleShape shell(radius, ShieldCirclePoints);

	shell.setOrigin({ radius, radius });
	shell.setPosition(center);
	shell.setFillColor(sf::Color(shellColor.r, shellColor.g, shellColor.b, static_cast<std::uint8_t>(27.f * pulse)));
	shell.setOutlineColor(sf::Color(outlineColor.r, outlineColor.g, outlineColor.b, static_cast<std::uint8_t>(58.f * pulse)));
	shell.setOutlineThickness(4.f);

	target.draw(shell, states);

	// 2) Three concentric outline rings, drawn with additive blending (colors
	// add instead of alpha-blending) so they build up into a soft glow around
	// the shell's edge. Each successive ring is bigger, dimmer, and thicker
	// than the last, which is what gives the glow its soft falloff.
	sf::RenderStates additiveStates(states);
	additiveStates.blendMode = sf::BlendAdd;
	for (int layer = 0; layer < 3; ++layer)
	{
		const float GlowRadius = radius + 2.f + layer * 3.f;
		sf::CircleShape glow(GlowRadius, ShieldCirclePoints);

		glow.setOrigin({ GlowRadius, GlowRadius });
		glow.setPosition(center);
		glow.setFillColor(sf::Color::Transparent);
		glow.setOutlineColor(sf::Color(glowColor.r, glowColor.g, glowColor.b,
			static_cast<std::uint8_t>((34.f - layer * 8.f) * pulse)));
		glow.setOutlineThickness(7.f + layer * 3.f);

		target.draw(glow, additiveStates);
	}

	// 3) The hex-grid overlay. GetHexGridLocalVertices did the expensive part
	// (once per distinct radius, cached); all that's left per frame is the
	// cheap part -- translate each cached local-space vertex by `center` to
	// place the grid at the shield's current world position, and recolor it.
	const sf::Color hexColor{ outlineColor.r,outlineColor.g,outlineColor.b,	static_cast<std::uint8_t>(hexOpacity * pulse) };
	const std::vector<sf::Vector2f>& localVertices = GetHexGridLocalVertices(radius);
	sf::VertexArray hexGrid(sf::PrimitiveType::Lines, localVertices.size());

	for (std::size_t index = 0u; index < localVertices.size(); index++)
		hexGrid[index] = sf::Vertex{ center + localVertices[index], hexColor };

	if (hexGrid.getVertexCount() > 0u)
		target.draw(hexGrid, additiveStates);
}
