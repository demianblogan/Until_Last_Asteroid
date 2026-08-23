#include "NineSliceFrame.h"

#include <algorithm>
#include <array>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Texture.hpp>

NineSliceFrame::NineSliceFrame(const sf::Texture& texture, sf::FloatRect target,
	unsigned int sourceBorder, sf::Vector2f targetBorder)
	: bounds(target)
{
	const sf::Vector2u size{ texture.getSize() };
	const int width{ static_cast<int>(size.x) };
	const int height{ static_cast<int>(size.y) };
	const int border{ static_cast<int>(std::min({
		sourceBorder, size.x / 2u, size.y / 2u })) };
	const std::array<int, 4> sx{ 0, border, width - border, width };
	const std::array<int, 4> sy{ 0, border, height - border, height };
	const std::array<float, 4> tx{ target.position.x,
		target.position.x + targetBorder.x,
		target.position.x + target.size.x - targetBorder.x,
		target.position.x + target.size.x };
	const std::array<float, 4> ty{ target.position.y,
		target.position.y + targetBorder.y,
		target.position.y + target.size.y - targetBorder.y,
		target.position.y + target.size.y };

	slices.reserve(9u);
	for (std::size_t row{ 0u }; row < 3u; ++row)
		for (std::size_t column{ 0u }; column < 3u; ++column)
		{
			const int sw{ sx[column + 1u] - sx[column] };
			const int sh{ sy[row + 1u] - sy[row] };
			slices.emplace_back(texture,
				sf::IntRect({ sx[column], sy[row] }, { sw, sh }));
			auto& slice{ slices.back() };
			slice.setPosition({ tx[column], ty[row] });
			slice.setScale({
				(tx[column + 1u] - tx[column]) / static_cast<float>(sw),
				(ty[row + 1u] - ty[row]) / static_cast<float>(sh) });
		}
}

void NineSliceFrame::SetColor(sf::Color color)
{
	for (auto& slice : slices) slice.setColor(color);
}

void NineSliceFrame::Draw(sf::RenderTarget& target) const
{
	for (const auto& slice : slices) target.draw(slice);
}

void NineSliceFrame::Draw(
	sf::RenderTarget& target, const sf::RenderStates& states) const
{
	for (const auto& slice : slices) target.draw(slice, states);
}

void NineSliceFrame::DrawBorder(
	sf::RenderTarget& target, const sf::RenderStates& states) const
{
	for (std::size_t index{ 0u }; index < slices.size(); ++index)
		if (index != 4u) target.draw(slices[index], states);
}

sf::FloatRect NineSliceFrame::GetBounds() const noexcept { return bounds; }
