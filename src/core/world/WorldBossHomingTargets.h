#pragma once

#include <array>
#include <cstddef>
#include <optional>

#include <SFML/System/Vector2.hpp>

// Up to 4 boss-supplied aim points that boss-owned homing projectiles seek,
// separate from the ordinary enemy/asteroid targets World::FindHomingTarget
// picks from among live entities.
class WorldBossHomingTargets
{
public:
	void Set(const std::array<std::optional<sf::Vector2f>, 4>& newTargets) noexcept;
	void Clear() noexcept;

	// Returns the index of the closest target within minimumDirectionDot of
	// direction from position, or nullopt if none qualify.
	[[nodiscard]] std::optional<std::size_t> FindClosest(
		const sf::Vector2f& position,
		const sf::Vector2f& direction,
		float minimumDirectionDot) const noexcept;
	[[nodiscard]] std::optional<sf::Vector2f> Get(std::size_t index) const noexcept;

private:
	std::array<std::optional<sf::Vector2f>, 4> targets{};
};
