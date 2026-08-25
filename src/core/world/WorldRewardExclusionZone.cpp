#include "WorldRewardExclusionZone.h"

#include <algorithm>
#include <cmath>

namespace
{
	sf::Vector2f Normalize(const sf::Vector2f& vector)
	{
		const float lengthSquared{ vector.x * vector.x + vector.y * vector.y };
		if (lengthSquared <= 0.0001f)
			return { 1.f, 0.f };
		return vector / std::sqrt(lengthSquared);
	}
}

void WorldRewardExclusionZone::Set(sf::Vector2f newCenter, float newRadius) noexcept
{
	center = newCenter;
	radius = std::max(0.f, newRadius);
}

void WorldRewardExclusionZone::Clear() noexcept
{
	center.reset();
	radius = 0.f;
}

sf::Vector2f WorldRewardExclusionZone::PushOutside(sf::Vector2f position) const noexcept
{
	if (!center || radius <= 0.f)
		return position;

	const sf::Vector2f offset{ position - *center };
	const float distanceSquared{ offset.x * offset.x + offset.y * offset.y };
	if (distanceSquared >= radius * radius)
		return position;

	return *center + Normalize(offset) * radius;
}
