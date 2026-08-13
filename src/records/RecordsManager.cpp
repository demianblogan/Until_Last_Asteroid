#include "RecordsManager.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

namespace
{
	using Json = nlohmann::json;

	Json Serialize(const GameRecords& records)
	{
		Json levels{ Json::object() };
		for (const auto& [level, score] : records.campaignLevelScores)
			levels[std::to_string(level)] = score;
		return {
			{ "format_version", GameRecords::FormatVersion },
			{ "campaign", { { "level_scores", std::move(levels) } } },
			{ "horde", { { "waves", records.hordeWaves }, { "score", records.hordeScore } } },
			{ "run", { { "seconds", records.runSeconds } } }
		};
	}

	GameRecords Deserialize(const Json& data)
	{
		if (!data.is_object() || data.value("format_version", 0) != GameRecords::FormatVersion)
			throw std::runtime_error("unsupported records format");
		GameRecords result;
		if (const auto campaign{ data.find("campaign") };
			campaign != data.end() && campaign->is_object())
		{
			if (const auto scores{ campaign->find("level_scores") };
				scores != campaign->end() && scores->is_object())
			{
				for (const auto& [levelText, score] : scores->items())
				{
					try
					{
						const int level{ std::stoi(levelText) };
						if (level > 0 && score.is_number_integer())
							result.campaignLevelScores[level] = std::max(0, score.get<int>());
					}
					catch (const std::exception&) {}
				}
			}
		}
		if (const auto horde{ data.find("horde") }; horde != data.end() && horde->is_object())
		{
			result.hordeWaves = std::max(0, horde->value("waves", 0));
			result.hordeScore = std::max(0, horde->value("score", 0));
		}
		if (const auto run{ data.find("run") }; run != data.end() && run->is_object())
			result.runSeconds = std::max(0, run->value("seconds", 0));
		return result;
	}

	bool ReplaceFile(const std::filesystem::path& temporaryPath,
		const std::filesystem::path& targetPath)
	{
		std::error_code error;
		if (!std::filesystem::exists(targetPath, error))
		{
			std::filesystem::rename(temporaryPath, targetPath, error);
			return !error;
		}
		std::filesystem::path backupPath{ targetPath };
		backupPath += ".bak";
		std::filesystem::remove(backupPath, error);
		error.clear();
		std::filesystem::rename(targetPath, backupPath, error);
		if (error) return false;
		std::filesystem::rename(temporaryPath, targetPath, error);
		if (error)
		{
			std::error_code restoreError;
			std::filesystem::rename(backupPath, targetPath, restoreError);
			return false;
		}
		std::filesystem::remove(backupPath, error);
		return true;
	}
}

RecordsManager::RecordsManager() : filePath(ResolvePath()) { Load(); }

const GameRecords& RecordsManager::Get() const noexcept { return records; }

int RecordsManager::GetCampaignLevelScore(int level) const noexcept
{
	const auto found{ records.campaignLevelScores.find(level) };
	return found == records.campaignLevelScores.end() ? 0 : found->second;
}

const std::filesystem::path& RecordsManager::GetFilePath() const noexcept { return filePath; }

bool RecordsManager::SubmitCampaignLevelScore(int level, int score)
{
	if (level <= 0 || score < 0) return false;
	int& best{ records.campaignLevelScores[level] };
	if (score <= best) return true;
	const int previous{ best };
	best = score;
	if (Save()) return true;
	best = previous;
	return false;
}

bool RecordsManager::MergeCampaignScores(const std::map<int, int>& scores)
{
	const GameRecords previous{ records };
	bool changed{ false };
	for (const auto& [level, score] : scores)
	{
		if (level <= 0 || score < 0) continue;
		int& best{ records.campaignLevelScores[level] };
		if (score > best) { best = score; changed = true; }
	}
	if (!changed || Save()) return true;
	records = previous;
	return false;
}

bool RecordsManager::Load()
{
	std::ifstream file(filePath);
	if (!file) { records = {}; return Save(); }
	try { records = Deserialize(Json::parse(file)); return true; }
	catch (const std::exception&) { records = {}; return PreserveCorruptFile(); }
}

bool RecordsManager::Save() const
{
	std::error_code error;
	std::filesystem::create_directories(filePath.parent_path(), error);
	if (error) return false;
	std::filesystem::path temporaryPath{ filePath };
	temporaryPath += ".tmp";
	{
		std::ofstream file(temporaryPath, std::ios::trunc);
		if (!file) return false;
		file << Serialize(records).dump(4) << '\n';
		if (!file) return false;
	}
	return ReplaceFile(temporaryPath, filePath);
}

std::filesystem::path RecordsManager::ResolvePath()
{
	char* localAppData{ nullptr };
	std::size_t length{ 0 };
	if (_dupenv_s(&localAppData, &length, "LOCALAPPDATA") == 0 && localAppData != nullptr)
	{
		const std::filesystem::path directory{ std::filesystem::path(localAppData) /
			"Alone Bull Company" / "Until Last Asteroid" };
		std::free(localAppData);
		return directory / "records.json";
	}
	std::free(localAppData);
	return std::filesystem::current_path() / "user_data" / "records.json";
}

bool RecordsManager::PreserveCorruptFile()
{
	std::error_code error;
	std::filesystem::path corruptPath{ filePath };
	corruptPath += ".corrupt";
	unsigned int suffix{ 1u };
	while (std::filesystem::exists(corruptPath, error) && !error)
	{
		corruptPath = filePath;
		corruptPath += ".corrupt." + std::to_string(suffix++);
	}
	if (error) return false;
	std::filesystem::rename(filePath, corruptPath, error);
	return !error;
}
