#pragma once

#include <map>
#include <string>
#include <vector>

#include "campaign/ShipUpgrades.h"

struct CampaignProgress
{
    static constexpr int FORMAT_VERSION{ 3 };

    int schemaVersion{ FORMAT_VERSION };
    bool tutorialCompleted{ false };
    bool campaignCompleted{ false };
    int currentLevel{ 1 };
    int highestUnlockedLevel{ 1 };
    std::vector<int> completedLevels;
    std::map<int, int> levelBestScores;
	int partsBalance{ 0 };
	std::vector<std::string> collectedPartIds;
	CampaignPhase phase{ CampaignPhase::Playing };
	ShipUpgradeRanks upgrades;
};
