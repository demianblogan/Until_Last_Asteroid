#pragma once

#include <filesystem>
#include <map>

struct GameRecords
{
	static constexpr int FormatVersion{ 1 };

	std::map<int, int> campaignLevelScores;
	int hordeWaves = 0;
	int hordeScore = 0;
	int runSeconds = 0;
};

class RecordsManager
{
public:
	RecordsManager();

	[[nodiscard]] const GameRecords& GetRecords() const noexcept;
	[[nodiscard]] int GetCampaignLevelScore(int level) const noexcept;
	bool SubmitCampaignLevelScore(int level, int score);
	bool SubmitHordeResult(int waves, int score);
	bool SubmitRunSeconds(int seconds);
	bool MergeCampaignScores(const std::map<int, int>& scores);

	bool LoadRecords();
	[[nodiscard]] bool SaveRecords() const;

private:
	[[nodiscard]] static std::filesystem::path ResolvePath();

	GameRecords records;
	std::filesystem::path recordsFilePath;
};
