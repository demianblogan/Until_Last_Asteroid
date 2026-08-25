#include "WorldStatisticsTracker.h"

void WorldStatisticsTracker::RecordAttackFired() noexcept
{
	++statistics.playerAttacksFired;
}

void WorldStatisticsTracker::RecordAttackHit() noexcept
{
	++statistics.playerAttacksHit;
}

void WorldStatisticsTracker::RecordShieldPickupCollected() noexcept
{
	++statistics.shieldPickupsCollected;
}

void WorldStatisticsTracker::RecordBigMeteorDestroyed() noexcept
{
	++statistics.bigMeteorsDestroyed;
}

void WorldStatisticsTracker::RecordSmallMeteorDestroyed() noexcept
{
	++statistics.smallMeteorsDestroyed;
}

void WorldStatisticsTracker::RecordShooterDestroyed() noexcept
{
	++statistics.shootersDestroyed;
}

const WorldStatistics& WorldStatisticsTracker::Get() const noexcept
{
	return statistics;
}

void WorldStatisticsTracker::Reset() noexcept
{
	statistics = {};
}
