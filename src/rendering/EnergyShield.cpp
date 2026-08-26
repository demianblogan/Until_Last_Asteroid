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
		const auto found{ cache.find(key) };
		if (found != cache.end())
			return found->second;

		constexpr float HexRadius = 9.f;
		constexpr float HorizontalSpacing = 16.f;
		constexpr float VerticalSpacing = 14.f;
		const int maximumRow = static_cast<int>(std::ceil(radius / VerticalSpacing));
		const int maximumColumn = static_cast<int>(std::ceil(radius / HorizontalSpacing));

		std::vector<sf::Vector2f> vertices;
		for (int row = -maximumRow; row <= maximumRow; ++row)
		{
			for (int column = -maximumColumn; column <= maximumColumn; ++column)
			{
				const float x = column * HorizontalSpacing + (row % 2 == 0 ? 0.f : 8.f);
				const float y = row * VerticalSpacing;
				if (x * x + y * y >
					(radius - HexRadius - 3.f) * (radius - HexRadius - 3.f))
				{
					continue;
				}

				std::array<sf::Vector2f, 6> corners;
				for (std::size_t corner = 0u; corner < corners.size(); ++corner)
				{
					const float angle = std::numbers::pi_v<float> / 6.f +
						static_cast<float>(corner) * std::numbers::pi_v<float> / 3.f;
					corners[corner] = sf::Vector2f{ x, y } + sf::Vector2f{
						std::cos(angle) * HexRadius,
						std::sin(angle) * HexRadius };
				}
				for (std::size_t corner = 0u; corner < corners.size(); ++corner)
				{
					vertices.push_back(corners[corner]);
					vertices.push_back(corners[(corner + 1u) % corners.size()]);
				}
			}
		}
		return cache.emplace(key, std::move(vertices)).first->second;
	}
}

void Rendering::DrawEnergyShield(
	sf::RenderTarget& target,
	sf::Vector2f center,
	float radius,
	float pulse,
	sf::Color shellColor,
	sf::Color outlineColor,
	sf::Color glowColor,
	float hexOpacity,
	sf::RenderStates states)
{
	constexpr std::size_t ShieldCirclePoints = 64u;
	sf::CircleShape shell(radius, ShieldCirclePoints);
	shell.setOrigin({ radius, radius });
	shell.setPosition(center);
	shell.setFillColor(sf::Color(shellColor.r, shellColor.g, shellColor.b,
		static_cast<std::uint8_t>(27.f * pulse)));
	shell.setOutlineColor(sf::Color(outlineColor.r, outlineColor.g, outlineColor.b,
		static_cast<std::uint8_t>(58.f * pulse)));
	shell.setOutlineThickness(4.f);
	target.draw(shell, states);

	sf::RenderStates additiveStates{ states };
	additiveStates.blendMode = sf::BlendAdd;
	for (int layer = 0; layer < 3; ++layer)
	{
		const float glowRadius = radius + 2.f + layer * 3.f;
		sf::CircleShape glow(glowRadius, ShieldCirclePoints);
		glow.setOrigin({ glowRadius, glowRadius });
		glow.setPosition(center);
		glow.setFillColor(sf::Color::Transparent);
		glow.setOutlineColor(sf::Color(glowColor.r, glowColor.g, glowColor.b,
			static_cast<std::uint8_t>((34.f - layer * 8.f) * pulse)));
		glow.setOutlineThickness(7.f + layer * 3.f);
		target.draw(glow, additiveStates);
	}

	const sf::Color hexColor{
		outlineColor.r,
		outlineColor.g,
		outlineColor.b,
		static_cast<std::uint8_t>(hexOpacity * pulse) };
	const std::vector<sf::Vector2f>& localVertices{ GetHexGridLocalVertices(radius) };
	sf::VertexArray hexGrid(sf::PrimitiveType::Lines, localVertices.size());
	for (std::size_t index = 0u; index < localVertices.size(); ++index)
		hexGrid[index] = sf::Vertex{ center + localVertices[index], hexColor };
	if (hexGrid.getVertexCount() > 0u)
		target.draw(hexGrid, additiveStates);
}
