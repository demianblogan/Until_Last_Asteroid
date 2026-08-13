#pragma once

#include <array>
#include <algorithm>

enum class CampaignPhase
{
	Playing,
	AwaitingUpgrades,
	ContentComplete
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
	int armor{ 0 };
	int engines{ 0 };
	int fireRate{ 0 };
	int bonusDuration{ 0 };
};

namespace ShipUpgradeRules
{
	inline constexpr int MaximumRank{ 4 };
	inline constexpr std::array<int, MaximumRank> RankCosts{ 1, 2, 3, 3 };
	inline constexpr float ArmorPerRank{ 0.25f };
	inline constexpr float SpeedPerRank{ 0.10f };
	inline constexpr float FireRatePerRank{ 0.15f };
	inline constexpr float BonusSecondsPerRank{ 1.f };

	inline int& GetRank(ShipUpgradeRanks& ranks, ShipUpgradeType type) noexcept
	{
		switch (type)
		{
		case ShipUpgradeType::Armor: return ranks.armor;
		case ShipUpgradeType::Engines: return ranks.engines;
		case ShipUpgradeType::FireRate: return ranks.fireRate;
		case ShipUpgradeType::BonusDuration: return ranks.bonusDuration;
		default: return ranks.armor;
		}
	}

	inline int GetRank(const ShipUpgradeRanks& ranks, ShipUpgradeType type) noexcept
	{
		ShipUpgradeRanks copy{ ranks };
		return GetRank(copy, type);
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
