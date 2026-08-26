#include "NineSliceFrame.h"

#include <algorithm>
#include <array>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace
{
	// The frame is cut into a 3x3 grid of slices: 4 unscaled corners, 4 edges
	// that stretch along one axis, and 1 center that stretches along both --
	// the classic "9-slice" technique for a resizable panel/frame background
	// that doesn't blur or distort its corners when scaled.
	constexpr std::size_t GridSize = 3u;
	constexpr std::size_t SliceCount = GridSize * GridSize;

	// The middle slice's index when the 9 slices are stored row-by-row
	// (index = row * GridSize + column): row 1, column 1 -> 1*3 + 1 = 4.
	constexpr std::size_t CenterSliceIndex = SliceCount / 2u;

	// GridSize cells need GridSize + 1 boundary coordinates to delimit them
	// (like fence posts: 3 gaps need 4 posts).
	constexpr std::size_t GridLineCount = GridSize + 1u;
}

namespace UI
{
	NineSliceFrame::NineSliceFrame(const sf::Texture& texture, sf::FloatRect destinationBounds,
		unsigned int textureBorderSize, sf::Vector2f targetBorderSize)
		: bounds(destinationBounds)
	{
		const sf::Vector2u textureSize = texture.getSize();
		const int textureWidth = static_cast<int>(textureSize.x);
		const int textureHeight = static_cast<int>(textureSize.y);

		// The requested border can't be honored as-is on a small texture (the two
		// opposing borders would overlap), so it's capped at half the texture's size.
		const int sourceBorderPixels =
			static_cast<int>(std::min({ textureBorderSize, textureSize.x / 2u, textureSize.y / 2u }));

		// The 4 x-boundaries, in source-texture pixels, that split the texture into
		// its 3 columns: [0, border), [border, width-border), [width-border, width).
		const std::array<int, GridLineCount> sourceColumnBounds =
		{
			0,
			sourceBorderPixels,
			textureWidth - sourceBorderPixels,
			textureWidth
		};

		// Same idea along the vertical axis, splitting the texture into 3 rows.
		const std::array<int, GridLineCount> sourceRowBounds =
		{
			0,
			sourceBorderPixels,
			textureHeight - sourceBorderPixels,
			textureHeight
		};

		// The matching 4 x-boundaries in the destination rectangle (on screen).
		// targetBorderSize is independent of sourceBorderPixels, since the frame can be
		// drawn larger or smaller than its source texture -- this is what lets the
		// corner slices stay crisp (unscaled) while everything between them stretches.
		const std::array<float, GridLineCount> targetColumnBounds =
		{
			destinationBounds.position.x,
			destinationBounds.position.x + targetBorderSize.x,
			destinationBounds.position.x + destinationBounds.size.x - targetBorderSize.x,
			destinationBounds.position.x + destinationBounds.size.x
		};

		const std::array<float, GridLineCount> targetRowBounds =
		{
			destinationBounds.position.y,
			destinationBounds.position.y + targetBorderSize.y,
			destinationBounds.position.y + destinationBounds.size.y - targetBorderSize.y,
			destinationBounds.position.y + destinationBounds.size.y
		};

		slices.reserve(SliceCount);

		for (std::size_t row = 0u; row < GridSize; row++)
		{
			for (std::size_t column = 0u; column < GridSize; column++)
			{
				// This slice's size in the source texture, i.e. the gap between its
				// two surrounding boundary lines.
				const int sliceSourceWidth = sourceColumnBounds[column + 1u] - sourceColumnBounds[column];
				const int sliceSourceHeight = sourceRowBounds[row + 1u] - sourceRowBounds[row];

				slices.emplace_back(texture, sf::IntRect(
					{ sourceColumnBounds[column], sourceRowBounds[row] },
					{ sliceSourceWidth, sliceSourceHeight }));

				sf::Sprite& slice = slices.back();
				slice.setPosition({ targetColumnBounds[column], targetRowBounds[row] });

				// Scale factor = how big this slice needs to appear on screen,
				// divided by how big it actually is in the source texture. Corner
				// slices end up at (near) 1.0 -- unscaled -- when targetBorderSize
				// matches sourceBorderPixels' on-screen size; edge/center slices
				// stretch to fill whatever space is left between the corners.
				slice.setScale(
					{
					(targetColumnBounds[column + 1u] - targetColumnBounds[column]) / static_cast<float>(sliceSourceWidth),
					(targetRowBounds[row + 1u] - targetRowBounds[row]) / static_cast<float>(sliceSourceHeight)
					});
			}
		}
	}

	void NineSliceFrame::SetColor(sf::Color color)
	{
		for (sf::Sprite& slice : slices)
			slice.setColor(color);
	}

	void NineSliceFrame::Draw(sf::RenderTarget& target) const
	{
		for (const sf::Sprite& slice : slices)
			target.draw(slice);
	}

	void NineSliceFrame::Draw(sf::RenderTarget& target, const sf::RenderStates& states) const
	{
		for (const sf::Sprite& slice : slices)
			target.draw(slice, states);
	}

	void NineSliceFrame::DrawBorder(sf::RenderTarget& target, const sf::RenderStates& states) const
	{
		for (std::size_t index = 0u; index < slices.size(); index++)
			if (index != CenterSliceIndex)
				target.draw(slices[index], states);
	}

	sf::FloatRect NineSliceFrame::GetBounds() const noexcept
	{
		return bounds;
	}
}