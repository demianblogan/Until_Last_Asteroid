#pragma once

#include <filesystem>
#include <map>

struct GameRecords
{
	static constexpr int FormatVersion{ 1 };

	std::map<int, int> campaignLevelScores;
	int hordeWaves{ 0 };
	int hordeScore{ 0 };
	int runSeconds{ 0 };
};

class RecordsManager
{
public:
	RecordsManager();

	[[nodiscard]] const GameRecords& Get() const noexcept;
	[[nodiscard]] int GetCampaignLevelScore(int level) const noexcept;
	[[nodiscard]] const std::filesystem::path& GetFilePath() const noexcept;
	bool SubmitCampaignLevelScore(int level, int score);
	bool SubmitHordeResult(int waves, int score);
	bool SubmitRunSeconds(int seconds);
	bool MergeCampaignScores(const std::map<int, int>& scores);
	bool Load();
	[[nodiscard]] bool Save() const;

private:
	[[nodiscard]] static std::filesystem::path ResolvePath();
	bool PreserveCorruptFile();

	GameRecords records;
	std::filesystem::path filePath;
};
