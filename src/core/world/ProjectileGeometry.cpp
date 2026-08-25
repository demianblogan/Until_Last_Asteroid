#include "ProjectileGeometry.h"

#include <algorithm>
#include <cmath>
#include <numbers>

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

namespace ProjectileGeometry
{
	std::optional<sf::Vector2f> TryCircleImpactDirection(
		sf::Vector2f center, float radius,
		sf::Vector2f projectilePosition, float projectileRadius)
	{
		const sf::Vector2f offset{ projectilePosition - center };
		const float reach{ radius + projectileRadius };
		if (offset.x * offset.x + offset.y * offset.y > reach * reach)
			return std::nullopt;
		return Normalize(offset);
	}

	std::optional<sf::Vector2f> TryAnnulusImpactDirection(
		sf::Vector2f center, float innerRadius, float outerRadius,
		sf::Vector2f projectilePosition, float projectileRadius)
	{
		const sf::Vector2f offset{ projectilePosition - center };
		const float distanceSquared{ offset.x * offset.x + offset.y * offset.y };
		const float outerReach{ outerRadius + projectileRadius };
		const float innerReach{ std::max(0.f, innerRadius - projectileRadius) };
		if (distanceSquared > outerReach * outerReach ||
			distanceSquared < innerReach * innerReach)
		{
			return std::nullopt;
		}
		return Normalize(offset);
	}

	std::optional<sf::Vector2f> TryDiamondFrameImpactDirection(
		sf::Vector2f center, float rotationDegrees, float vertexRadius, float halfThickness,
		sf::Vector2f projectilePosition, float projectileRadius)
	{
		const float rotation{ -rotationDegrees * std::numbers::pi_v<float> / 180.f };
		const float cosine{ std::cos(rotation) };
		const float sine{ std::sin(rotation) };

		const sf::Vector2f offset{ projectilePosition - center };
		const sf::Vector2f local{
			offset.x * cosine - offset.y * sine,
			offset.x * sine + offset.y * cosine };
		const float diamondDistance{ std::abs(local.x) + std::abs(local.y) };
		if (std::abs(diamondDistance - vertexRadius) > halfThickness + projectileRadius)
			return std::nullopt;

		return Normalize(offset);
	}

	std::optional<sf::Vector2f> TrySegmentImpactPoint(
		sf::Vector2f start, sf::Vector2f end, float segmentWidth,
		sf::Vector2f targetPosition, float targetRadius)
	{
		const sf::Vector2f segment{ end - start };
		const float lengthSquared{ segment.x * segment.x + segment.y * segment.y };
		if (lengthSquared <= 0.001f)
			return std::nullopt;

		const sf::Vector2f relative{ targetPosition - start };
		const float projection{ std::clamp(
			(relative.x * segment.x + relative.y * segment.y) / lengthSquared, 0.f, 1.f) };
		const sf::Vector2f impactPoint{ start + segment * projection };

		const sf::Vector2f offset{ targetPosition - impactPoint };
		const float hitRadius{ targetRadius + segmentWidth * 0.5f };
		if (offset.x * offset.x + offset.y * offset.y > hitRadius * hitRadius)
			return std::nullopt;

		return impactPoint;
	}
}
