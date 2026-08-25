#pragma once

#include <optional>

#include <SFML/System/Vector2.hpp>

// A boss-defined circle that dropped rewards must not land inside (e.g. an
// arena hazard a pickup would otherwise spawn on top of). Inactive until Set
// is called.
class WorldRewardExclusionZone
{
public:
	void Set(sf::Vector2f center, float radius) noexcept;
	void Clear() noexcept;

	// Returns position unchanged if the zone is inactive or position is
	// already outside it; otherwise returns position pushed out to the
	// zone's edge, along the line from the zone's center through position.
	[[nodiscard]] sf::Vector2f PushOutside(sf::Vector2f position) const noexcept;

private:
	std::optional<sf::Vector2f> center;
	float radius = 0.f;
};
