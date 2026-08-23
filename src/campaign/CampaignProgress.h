#pragma once

#include <map>
#include <string>
#include <vector>

#include "campaign/ShipUpgrades.h"

struct CampaignProgress
{
	static constexpr int FormatVersion = 4;

	bool isTutorialCompleted = false;
	bool isCampaignCompleted = false;
	bool isTutorialSkipped = false;

	int currentLevel = 1;
	int highestUnlockedLevel = 1;
	std::vector<int> completedLevels;
	std::map<int, int> levelBestScores;

	int partsBalance = 0;
	std::vector<std::string> collectedPartIDs;
	ShipUpgradeRanks upgrades;

	CampaignPhase phase = CampaignPhase::Playing;
	bool isNoDeathAchievementEligible = true;
};