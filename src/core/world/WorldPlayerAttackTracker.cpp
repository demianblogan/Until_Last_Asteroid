#include "WorldPlayerAttackTracker.h"

std::uint64_t WorldPlayerAttackTracker::Begin() noexcept
{
	return nextAttackID++;
}

bool WorldPlayerAttackTracker::RegisterHit(std::uint64_t attackID) noexcept
{
	if (attackID == 0u)
		return false;
	return successfulAttacks.insert(attackID).second;
}

void WorldPlayerAttackTracker::Reset() noexcept
{
	nextAttackID = 1u;
	successfulAttacks.clear();
}
