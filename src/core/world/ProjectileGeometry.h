#pragma once

#include <optional>

#include <SFML/System/Vector2.hpp>

// Pure hit-test math for the boss-attack shapes that sweep away player
// projectiles (circle blasts, ring/annulus waves, rotating diamond frames).
// Each function returns the normalized impact direction (projectile center
// minus shape center) if the projectile overlaps the shape, or nullopt if it
// doesn't -- callers own what happens on a hit (destroying the projectile,
// playing sounds, raising effect events).
namespace ProjectileGeometry
{
	[[nodiscard]] std::optional<sf::Vector2f> TryCircleImpactDirection(sf::Vector2f center, float radius,
		sf::Vector2f projectilePosition, float projectileRadius);

	[[nodiscard]] std::optional<sf::Vector2f> TryAnnulusImpactDirection(sf::Vector2f center, float innerRadius,
		float outerRadius, sf::Vector2f projectilePosition, float projectileRadius);

	[[nodiscard]] std::optional<sf::Vector2f> TryDiamondFrameImpactDirection(sf::Vector2f center, float rotationDegrees,
		float vertexRadius, float halfThickness, sf::Vector2f projectilePosition, float projectileRadius);

	// For a beam/laser drawn from start to end with the given width: the point
	// on the segment closest to targetPosition, if targetPosition (treated as a
	// circle of targetRadius) overlaps the beam; nullopt if it doesn't, or if
	// start and end are coincident.
	[[nodiscard]] std::optional<sf::Vector2f> TrySegmentImpactPoint(sf::Vector2f start, sf::Vector2f end, float segmentWidth,
		sf::Vector2f targetPosition, float targetRadius);
}