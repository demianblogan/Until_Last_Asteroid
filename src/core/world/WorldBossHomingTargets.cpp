#include "WorldBossHomingTargets.h"

#include <cmath>
#include <limits>

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

void WorldBossHomingTargets::Set(
	const std::array<std::optional<sf::Vector2f>, 4>& newTargets) noexcept
{
	targets = newTargets;
}

void WorldBossHomingTargets::Clear() noexcept
{
	targets.fill(std::nullopt);
}

std::optional<std::size_t> WorldBossHomingTargets::FindClosest(
	const sf::Vector2f& position,
	const sf::Vector2f& direction,
	float minimumDirectionDot) const noexcept
{
	const sf::Vector2f normalizedDirection{ Normalize(direction) };
	std::optional<std::size_t> closestTarget;
	float closestDistanceSquared{ std::numeric_limits<float>::max() };
	for (std::size_t index{ 0u }; index < targets.size(); ++index)
	{
		if (!targets[index])
			continue;
		const sf::Vector2f offset{ *targets[index] - position };
		const float distanceSquared{ offset.x * offset.x + offset.y * offset.y };
		if (distanceSquared <= 0.0001f || distanceSquared >= closestDistanceSquared)
			continue;
		const float directionDot{
			(offset.x * normalizedDirection.x + offset.y * normalizedDirection.y) /
			std::sqrt(distanceSquared) };
		if (directionDot < minimumDirectionDot)
			continue;
		closestTarget = index;
		closestDistanceSquared = distanceSquared;
	}
	return closestTarget;
}

std::optional<sf::Vector2f> WorldBossHomingTargets::Get(std::size_t index) const noexcept
{
	return index < targets.size() ? targets[index] : std::nullopt;
}
