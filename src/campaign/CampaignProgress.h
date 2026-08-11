#pragma once

#include <map>
#include <vector>

struct CampaignProgress
{
    static constexpr int FORMAT_VERSION{ 1 };

    int schemaVersion{ FORMAT_VERSION };
    bool tutorialCompleted{ false };
    bool campaignCompleted{ false };
    int currentLevel{ 1 };
    int highestUnlockedLevel{ 1 };
    std::vector<int> completedLevels;
    std::map<int, int> levelBestScores;
    int campaignScore{ 0 };
};
