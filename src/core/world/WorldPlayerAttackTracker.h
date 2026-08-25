#pragma once

#include <cstdint>
#include <unordered_set>

// Issues unique IDs for player attacks (shots, laser beams...) and lets
// callers register a hit against one exactly once, even if the same attack
// can report a hit multiple times (e.g. a laser beam ticking every frame it
// stays on an enemy) -- RegisterHit only returns true the first time.
class WorldPlayerAttackTracker
{
public:
	[[nodiscard]] std::uint64_t Begin() noexcept;
	[[nodiscard]] bool RegisterHit(std::uint64_t attackID) noexcept;
	void Reset() noexcept;

private:
	std::uint64_t nextAttackID = 1u;
	std::unordered_set<std::uint64_t> successfulAttacks;
};
