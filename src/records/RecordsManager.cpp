#include "RecordsManager.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

#include "utils/AppDataPath.h"
#include "utils/SafeFileWrite.h"

namespace
{
	using Json = nlohmann::json;
	using SafeFileWrite::HasFailed;

	// Same reasoning as JSONIndentWidth in CampaignSaveManager.cpp: this is
	// the pretty-print indent width for the saved JSON, not an arbitrary
	// number.
	constexpr int JSONIndentWidth = 4;

	Json Serialize(const GameRecords& records)
	{
		Json levels(Json::object());

		for (const auto& [level, score] : records.campaignLevelScores)
			levels[std::to_string(level)] = score;

		return
		{
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
		if (const auto campaign(data.find("campaign"));
			campaign != data.end() && campaign->is_object())
		{
			if (const auto scores(campaign->find("level_scores"));
				scores != campaign->end() && scores->is_object())
			{
				for (const auto& [levelText, score] : scores->items())
				{
					try
					{
						const int level = std::stoi(levelText);
						if (level > 0 && score.is_number_integer())
							result.campaignLevelScores[level] = std::max(0, score.get<int>());
					}
					catch (const std::exception&)
					{
					}
				}
			}
		}
		if (const auto horde(data.find("horde")); horde != data.end() && horde->is_object())
		{
			result.hordeWaves = std::max(0, horde->value("waves", 0));
			result.hordeScore = std::max(0, horde->value("score", 0));
		}

		if (const auto run(data.find("run")); run != data.end() && run->is_object())
			result.runSeconds = std::max(0, run->value("seconds", 0));

		return result;
	}
}

RecordsManager::RecordsManager()
	: recordsFilePath(ResolvePath())
{
	LoadRecords();
}

const GameRecords& RecordsManager::GetRecords() const noexcept
{
	return records;
}

int RecordsManager::GetCampaignLevelScore(int level) const noexcept
{
	const auto found(records.campaignLevelScores.find(level));
	return found == records.campaignLevelScores.end() ? 0 : found->second;
}

bool RecordsManager::SubmitCampaignLevelScore(int level, int score)
{
	if (level <= 0 || score < 0)
		return false;

	int& bestScore = records.campaignLevelScores[level];

	if (score <= bestScore)
		return true;

	const int previousScore = bestScore;
	bestScore = score;

	if (SaveRecords())
		return true;

	bestScore = previousScore;

	return false;
}

bool RecordsManager::SubmitHordeResult(int waves, int score)
{
	if (waves < 0 || score < 0)
		return false;

	const GameRecords previousRecords(records);

	records.hordeWaves = std::max(records.hordeWaves, waves);
	records.hordeScore = std::max(records.hordeScore, score);

	if (records.hordeWaves == previousRecords.hordeWaves && records.hordeScore == previousRecords.hordeScore)
	{
		return true;
	}

	if (SaveRecords())
		return true;

	records = previousRecords;

	return false;
}

bool RecordsManager::SubmitRunSeconds(int seconds)
{
	if (seconds < 0)
		return false;

	if (seconds <= records.runSeconds)
		return true;

	const int previousSeconds = records.runSeconds;
	records.runSeconds = seconds;

	if (SaveRecords())
		return true;

	records.runSeconds = previousSeconds;

	return false;
}

bool RecordsManager::MergeCampaignScores(const std::map<int, int>& scores)
{
	const GameRecords previousRecords(records);
	bool isScoreChanged = false;

	for (const auto& [level, score] : scores)
	{
		if (level <= 0 || score < 0)
			continue;

		int& bestScore = records.campaignLevelScores[level];
		if (score > bestScore)
		{
			bestScore = score;
			isScoreChanged = true;
		}
	}

	if (!isScoreChanged || SaveRecords())
		return true;

	records = previousRecords;

	return false;
}

bool RecordsManager::LoadRecords()
{
	std::ifstream file(recordsFilePath);
	if (!file.is_open())
	{
		records = {};
		return SaveRecords();
	}

	try
	{
		records = Deserialize(Json::parse(file));
		return true;
	}
	catch (const std::exception&)
	{
		records = {};
		return SafeFileWrite::PreserveCorruptFile(recordsFilePath);
	}
}

bool RecordsManager::SaveRecords() const
{
	std::error_code error;

	std::filesystem::create_directories(recordsFilePath.parent_path(), error);
	if (HasFailed(error))
		return false;

	std::filesystem::path temporaryPath(recordsFilePath);
	temporaryPath += ".tmp";

	{
		std::ofstream file(temporaryPath, std::ios::trunc);
		if (!file.is_open())
			return false;

		file << Serialize(records).dump(JSONIndentWidth) << '\n';

		if (!file)
			return false;
	}

	return SafeFileWrite::ReplaceFileAtomically(temporaryPath, recordsFilePath);
}

std::filesystem::path RecordsManager::ResolvePath()
{
	return AppDataPath::Resolve("records.json");
}