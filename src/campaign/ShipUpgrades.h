#pragma once

#include <array>
#include <algorithm>
#include <cassert>

// Persisted player-progress state, saved to and loaded from the campaign
// save file (see CampaignSaveManager) -- NOT which UI screen is currently
// showing. It survives app restarts and only drives which screen the
// campaign menu redirects the player to next; it isn't a screen itself.
enum class CampaignPhase
{
	// Normal state: nothing is owed. The player can be anywhere -- campaign
	// menu, gameplay, paused, options -- this doesn't track that.
	Playing,

	// Set right after finishing a level (not a replay) and cleared once the
	// player has visited the Ship Upgrades screen for it. Can persist across
	// app restarts if the player quits before visiting Ship Upgrades -- the
	// campaign menu uses this to redirect them there first.
	AwaitingUpgrades,

	// Permanent end state: the final level has been completed AND the
	// player has already been through the post-campaign Ship Upgrades
	// visit. Once set, it never reverts to Playing.
	Finished
};

enum class ShipUpgradeType
{
	Armor,
	Engines,
	FireRate,
	BonusDuration,
	Count
};

struct ShipUpgradeRanks
{
	int armor = 0;
	int engines = 0;
	int fireRate = 0;
	int bonusDuration = 0;
};

namespace ShipUpgradeRules
{
	inline constexpr int MaximumRank = 4;
	inline constexpr std::array<int, MaximumRank> RankCosts = { 1, 2, 3, 3 };
	inline constexpr float ArmorPerRank = 0.25f;
	inline constexpr float SpeedPerRank = 0.10f;
	inline constexpr float FireRatePerRank = 0.15f;
	inline constexpr float BonusSecondsPerRank = 1.f;

	inline int& GetRank(ShipUpgradeRanks& ranks, ShipUpgradeType type) noexcept
	{
		switch (type)
		{
		case ShipUpgradeType::Armor:
			return ranks.armor;
		case ShipUpgradeType::Engines:
			return ranks.engines;
		case ShipUpgradeType::FireRate:
			return ranks.fireRate;
		case ShipUpgradeType::BonusDuration:
			return ranks.bonusDuration;

		default:
			// Only ShipUpgradeType::Count (or a bad cast) lands here -- never
			// a real upgrade type from current call sites. Asserts loudly in
			// Debug so a future bug (new enumerator added without updating
			// this switch, off-by-one in a loop, etc.) is caught right here
			// instead of silently returning/writing through the wrong rank.
			// Falls back to armor in Release, since this function is
			// noexcept and can't throw to report the problem instead.
			assert(false && "Unhandled ShipUpgradeType in GetRank");
			return ranks.armor;
		}
	}

	// Deliberately not implemented as "copy ranks, then call the non-const
	// overload": that would need a non-const reference to `ranks`, and
	// getting one via const_cast here would be undefined behavior if the
	// caller's ShipUpgradeRanks is genuinely const (not just accessed
	// through a const&). Duplicating the switch is a few extra lines but
	// keeps this entirely on the safe side of const-correctness.
	inline int GetRank(const ShipUpgradeRanks& ranks, ShipUpgradeType type) noexcept
	{
		switch (type)
		{
		case ShipUpgradeType::Armor:
			return ranks.armor;
		case ShipUpgradeType::Engines:
			return ranks.engines;
		case ShipUpgradeType::FireRate:
			return ranks.fireRate;
		case ShipUpgradeType::BonusDuration:
			return ranks.bonusDuration;

		default:
			assert(false && "Unhandled ShipUpgradeType in GetRank");
			return ranks.armor;
		}
	}

	inline int GetNextCost(int rank) noexcept
	{
		return rank >= 0 && rank < MaximumRank ? RankCosts[rank] : 0;
	}

	inline void Clamp(ShipUpgradeRanks& ranks) noexcept
	{
		ranks.armor = std::clamp(ranks.armor, 0, MaximumRank);
		ranks.engines = std::clamp(ranks.engines, 0, MaximumRank);
		ranks.fireRate = std::clamp(ranks.fireRate, 0, MaximumRank);
		ranks.bonusDuration = std::clamp(ranks.bonusDuration, 0, MaximumRank);
	}
}