#pragma once

// A snapshot of gameplay statistics for the current run, shown on the
// end-of-run/results screens. Defined outside WorldStatisticsTracker (rather
// than nested in it) so World can still expose it as World::Statistics via a
// type alias, unchanged for existing callers.
struct WorldStatistics
{
	unsigned int playerAttacksFired = 0u;
	unsigned int playerAttacksHit = 0u;
	unsigned int bigMeteorsDestroyed = 0u;
	unsigned int smallMeteorsDestroyed = 0u;
	unsigned int shootersDestroyed = 0u;
	unsigned int shieldPickupsCollected = 0u;
};

// Owns the run's gameplay statistics and the handful of increment points
// scattered across World's collision/pickup/attack handling.
class WorldStatisticsTracker
{
public:
	void RecordAttackFired() noexcept;
	void RecordAttackHit() noexcept;
	void RecordShieldPickupCollected() noexcept;
	void RecordBigMeteorDestroyed() noexcept;
	void RecordSmallMeteorDestroyed() noexcept;
	void RecordShooterDestroyed() noexcept;

	[[nodiscard]] const WorldStatistics& Get() const noexcept;
	void Reset() noexcept;

private:
	WorldStatistics statistics;
};
