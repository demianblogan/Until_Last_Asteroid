#include "WorldBossHomingTargets.h"

#include <cmath>
#include <limits>

#include "utils/VectorMath.h"

namespace
{
	// Guards against a target that coincides (or nearly does) with position --
	// a zero offset has no meaningful direction to dot against.
	constexpr float CoincidentTargetEpsilonSquared = 0.0001f;
}

void WorldBossHomingTargets::Set(const std::array<std::optional<sf::Vector2f>, MaximumTargets>& newTargets) noexcept
{
	targets = newTargets;
}

void WorldBossHomingTargets::Clear() noexcept
{
	targets.fill(std::nullopt);
}

std::optional<std::size_t> WorldBossHomingTargets::FindClosest(const sf::Vector2f& position, const sf::Vector2f& direction,
	float minimumDirectionDot) const noexcept
{
	const sf::Vector2f normalizedDirection{ VectorMath::Normalize(direction) };
	std::optional<std::size_t> closestTarget;
	float closestDistanceSquared = std::numeric_limits<float>::max();

	for (std::size_t index = 0u; index < targets.size(); index++)
	{
		if (!targets[index])
			continue;

		const sf::Vector2f offset{ *targets[index] - position };
		const float distanceSquared = offset.x * offset.x + offset.y * offset.y;
		if (distanceSquared <= CoincidentTargetEpsilonSquared || distanceSquared >= closestDistanceSquared)
			continue;

		const float directionDot =
			(offset.x * normalizedDirection.x + offset.y * normalizedDirection.y) / std::sqrt(distanceSquared);

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