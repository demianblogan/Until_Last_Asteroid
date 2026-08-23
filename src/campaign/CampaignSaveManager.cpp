#include "CampaignSaveManager.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

#include <nlohmann/json.hpp>

#include "utils/AppDataPath.h"
#include "utils/SafeFileWrite.h"

namespace
{
	using Json = nlohmann::json;
	using SafeFileWrite::HasFailed;

	// Same reasoning as JSONIndentWidth in AchievementManager.cpp: this is
	// the pretty-print indent width for the saved JSON, not an arbitrary
	// number.
	constexpr int JSONIndentWidth = 4;

	const char* SerializePhase(CampaignPhase phase)
	{
		switch (phase)
		{
		case CampaignPhase::Playing:
			return "playing";
		case CampaignPhase::AwaitingUpgrades:
			return "awaiting_upgrades";
		case CampaignPhase::Finished:
			return "content_complete";

		default:
			// Every real CampaignPhase value is handled explicitly above --
			// this is only reachable via a bad enum value (e.g. a future
			// enumerator added without updating this switch). Asserts loudly
			// in Debug instead of silently writing "playing" to the save
			// file for a phase that was never actually Playing.
			assert(false && "Unhandled CampaignPhase in SerializePhase");
			return "playing";
		}
	}

	// Unlike SerializePhase above, an unrecognized string here is an
	// expected, legitimate case, not a programmer bug: this parses a string
	// that came from an external save file, which could be hand-edited,
	// corrupted, or written by a newer game version with a phase this build
	// doesn't know about. Falling back to Playing (the safe, "nothing owed"
	// state) is the correct resilience behavior -- asserting/throwing here
	// would turn one unrecognized word into a reason to quarantine the
	// player's entire save (see Load()'s catch block).
	CampaignPhase DeserializePhase(std::string_view value)
	{
		if (value == "awaiting_upgrades")
			return CampaignPhase::AwaitingUpgrades;
		else if (value == "content_complete")
			return CampaignPhase::Finished;
		else
			return CampaignPhase::Playing;
	}

	Json Serialize(const CampaignProgress& progress)
	{
		Json bestScores = Json::object();

		for (const auto& [level, score] : progress.levelBestScores)
			bestScores[std::to_string(level)] = score;

		return
		{
			{ "schema_version", CampaignProgress::FormatVersion },
			{ "tutorial_completed", progress.isTutorialCompleted },
			{ "campaign_completed", progress.isCampaignCompleted },
			{ "current_level", progress.currentLevel },
			{ "highest_unlocked_level", progress.highestUnlockedLevel },
			{ "completed_levels", progress.completedLevels },
			{ "level_best_scores", std::move(bestScores) },
			{ "parts_balance", progress.partsBalance },
			{ "collected_part_ids", progress.collectedPartIDs },
			{ "campaign_phase", SerializePhase(progress.phase) },
			{ "no_death_achievement_eligible", progress.isNoDeathAchievementEligible },
			{ "tutorial_skipped", progress.isTutorialSkipped },
			{ "ship_upgrades",
				{
					{ "armor", progress.upgrades.armor },
					{ "engines", progress.upgrades.engines },
					{ "fire_rate", progress.upgrades.fireRate },
					{ "bonus_duration", progress.upgrades.bonusDuration }
				}
			}
		};
	}

	CampaignProgress Deserialize(const Json& data)
	{
		// 302 isn't arbitrary -- it's nlohmann::json's own documented
		// exception id for "type must be object, but is <actual type>"
		// (their scheme: 1xx parse_error, 2xx invalid_iterator, 3xx
		// type_error, 4xx out_of_range). Reusing it here throws the exact
		// same kind of type_error the library itself would throw for this
		// situation, so callers catching nlohmann's exceptions elsewhere see
		// consistent, recognizable error ids instead of a one-off custom one.
		constexpr int NotAnObjectErrorID = 302;
		if (!data.is_object())
			throw Json::type_error::create(
				NotAnObjectErrorID, "campaign save must be an object", &data);

		const int schemaVersion = data.at("schema_version").get<int>();
		constexpr int OldestSupportedVersion = 3;

		if (schemaVersion < OldestSupportedVersion || schemaVersion > CampaignProgress::FormatVersion)
			throw std::runtime_error("unsupported campaign save version");

		CampaignProgress result;
		result.isTutorialCompleted = data.value("tutorial_completed", false);
		result.isCampaignCompleted = data.value("campaign_completed", false);
		result.currentLevel = std::max(1, data.value("current_level", 1));
		result.highestUnlockedLevel = std::max(1, data.value("highest_unlocked_level", 1));
		result.partsBalance = std::max(0, data.value("parts_balance", 0));
		result.phase = DeserializePhase(data.value("campaign_phase", "playing"));
		result.isNoDeathAchievementEligible = schemaVersion >= 4
			? data.value("no_death_achievement_eligible", false)
			: false;
		result.isTutorialSkipped = data.value("tutorial_skipped", false);

		if (const auto upgrades = data.find("ship_upgrades"); upgrades != data.end() && upgrades->is_object())
		{
			result.upgrades.armor = upgrades->value("armor", 0);
			result.upgrades.engines = upgrades->value("engines", 0);
			result.upgrades.fireRate = upgrades->value("fire_rate", 0);
			result.upgrades.bonusDuration = upgrades->value("bonus_duration", 0);

			ShipUpgradeRules::Clamp(result.upgrades);
		}

		if (const auto parts = data.find("collected_part_ids"); parts != data.end() && parts->is_array())
		{
			for (const Json& id : *parts)
			{
				if (id.is_string() && !id.get_ref<const std::string&>().empty())
					result.collectedPartIDs.push_back(id.get<std::string>());
			}
		}

		if (const auto completedLevels = data.find("completed_levels");
			completedLevels != data.end() && completedLevels->is_array())
		{
			for (const Json& level : *completedLevels)
			{
				if (level.is_number_integer() && level.get<int>() > 0)
					result.completedLevels.push_back(level.get<int>());
			}
		}

		if (const auto scores = data.find("level_best_scores"); scores != data.end() && scores->is_object())
		{
			for (const auto& [levelText, score] : scores->items())
			{
				try
				{
					const int level = std::stoi(levelText);
					if (level > 0 && score.is_number_integer())
						result.levelBestScores[level] = std::max(0, score.get<int>());
				}
				catch (const std::exception&)
				{
					// std::stoi throws if levelText isn't a valid integer --
					// an expected possibility here since this key came from
					// an external (possibly hand-edited or corrupted) save
					// file. Skipping just this one malformed score entry and
					// continuing with the rest is the point: letting the
					// exception escape would fail this whole Deserialize()
					// call, which quarantines the player's entire save (see
					// Load()'s catch block) over a single bad key.
				}
			}
		}

		return result;
	}

}

CampaignSaveManager::CampaignSaveManager()
	: saveFilePath(ResolveSavePath())
{
	Load();
}

bool CampaignSaveManager::HasSave() const noexcept
{
	return progress.has_value();
}

const CampaignProgress* CampaignSaveManager::GetProgress() const noexcept
{
	return progress ? &progress.value() : nullptr;
}

CampaignProgress* CampaignSaveManager::EditProgress() noexcept
{
	return progress ? &progress.value() : nullptr;
}

bool CampaignSaveManager::Load()
{
	std::ifstream file(saveFilePath);
	if (!file.is_open())
	{
		progress.reset();
		return true;
	}

	try
	{
		progress = Deserialize(Json::parse(file));
		return true;
	}
	catch (const std::exception&)
	{
		progress.reset();
		return SafeFileWrite::PreserveCorruptFile(saveFilePath);
	}
}

bool CampaignSaveManager::StartNewCampaign()
{
	progress = CampaignProgress{};
	if (Save())
		return true;

	progress.reset();
	return false;
}

void CampaignSaveManager::UnlockNewLevel(int availableLevels)
{
	if (!progress || availableLevels <= 0)
		return;

	// The player is only owed a newly unlocked level if all three hold:
	const bool hasNewLevelToUnlock =
		// (1) they'd already finished everything that used to exist --
		// nothing to unlock if the campaign is still in progress.
		progress->phase == CampaignPhase::Finished &&
		// (2) this build actually has more levels than that now -- nothing
		// to unlock if no update has added new content since they finished.
		progress->currentLevel < availableLevels &&
		// (3) the level they stopped at is genuinely marked completed
		// (defensive sanity check against a corrupted/unexpected save state).
		std::ranges::find(progress->completedLevels, progress->currentLevel) !=
			progress->completedLevels.end();

	if (!hasNewLevelToUnlock)
		return;

	const CampaignProgress progressBeforeUnlock = *progress;
	progress->currentLevel++;
	progress->highestUnlockedLevel = std::max(progress->highestUnlockedLevel, progress->currentLevel);
	progress->isCampaignCompleted = false;
	progress->phase = CampaignPhase::Playing;

	// If the save fails, roll back to keep the in-memory progress from
	// drifting out of sync with what's actually on disk. The player will
	// simply be offered this same unlock again on the next launch.
	if (!Save())
		progress = progressBeforeUnlock;
}

bool CampaignSaveManager::Save() const
{
	if (!progress)
		return false;

	std::error_code error;
	std::filesystem::create_directories(saveFilePath.parent_path(), error);
	if (HasFailed(error))
		return false;

	std::filesystem::path temporaryPath(saveFilePath);
	temporaryPath += ".tmp";
	{
		std::ofstream file(temporaryPath, std::ios::trunc);
		if (!file.is_open())
			return false;

		file << Serialize(progress.value()).dump(JSONIndentWidth) << '\n';
		if (!file)
			return false;
	}

	return SafeFileWrite::ReplaceFileAtomically(temporaryPath, saveFilePath);
}

bool CampaignSaveManager::DeleteSave()
{
	std::error_code error;
	const bool isSaveFileExisted = std::filesystem::exists(saveFilePath, error);
	if (HasFailed(error))
		return false;

	if (isSaveFileExisted && !std::filesystem::remove(saveFilePath, error))
		return false;
	if (HasFailed(error))
		return false;

	progress.reset();

	return true;
}

std::filesystem::path CampaignSaveManager::ResolveSavePath()
{
	return AppDataPath::Resolve("campaign.json");
}