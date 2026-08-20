#include "CampaignSaveManager.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>

namespace
{
    using Json = nlohmann::json;

	const char* SerializePhase(CampaignPhase phase)
	{
		switch (phase)
		{
		case CampaignPhase::AwaitingUpgrades: return "awaiting_upgrades";
		case CampaignPhase::ContentComplete: return "content_complete";
		default: return "playing";
		}
	}

	CampaignPhase DeserializePhase(const std::string& value)
	{
		if (value == "awaiting_upgrades") return CampaignPhase::AwaitingUpgrades;
		if (value == "content_complete") return CampaignPhase::ContentComplete;
		return CampaignPhase::Playing;
	}

    Json Serialize(const CampaignProgress& progress)
    {
        Json bestScores{ Json::object() };
        for (const auto& [level, score] : progress.levelBestScores)
            bestScores[std::to_string(level)] = score;

        return {
            { "schema_version", CampaignProgress::FORMAT_VERSION },
            { "tutorial_completed", progress.tutorialCompleted },
            { "campaign_completed", progress.campaignCompleted },
            { "current_level", progress.currentLevel },
            { "highest_unlocked_level", progress.highestUnlockedLevel },
            { "completed_levels", progress.completedLevels },
            { "level_best_scores", std::move(bestScores) },
			{ "parts_balance", progress.partsBalance },
			{ "collected_part_ids", progress.collectedPartIds },
			{ "campaign_phase", SerializePhase(progress.phase) },
			{ "ship_upgrades", {
				{ "armor", progress.upgrades.armor },
				{ "engines", progress.upgrades.engines },
				{ "fire_rate", progress.upgrades.fireRate },
				{ "bonus_duration", progress.upgrades.bonusDuration }
			} }
        };
    }

    CampaignProgress Deserialize(const Json& data)
    {
        if (!data.is_object())
            throw Json::type_error::create(302, "campaign save must be an object", &data);

        const int version{ data.at("schema_version").get<int>() };
        if (version != CampaignProgress::FORMAT_VERSION)
            throw std::runtime_error("unsupported campaign save version");

        CampaignProgress result;
        result.schemaVersion = version;
        result.tutorialCompleted = data.value("tutorial_completed", false);
        result.campaignCompleted = data.value("campaign_completed", false);
        result.currentLevel = std::max(1, data.value("current_level", 1));
        result.highestUnlockedLevel = std::max(1, data.value("highest_unlocked_level", 1));
		result.partsBalance = std::max(0, data.value("parts_balance", 0));
		result.phase = DeserializePhase(data.value("campaign_phase", "playing"));
		if (const auto upgrades{ data.find("ship_upgrades") };
			upgrades != data.end() && upgrades->is_object())
		{
			result.upgrades.armor = upgrades->value("armor", 0);
			result.upgrades.engines = upgrades->value("engines", 0);
			result.upgrades.fireRate = upgrades->value("fire_rate", 0);
			result.upgrades.bonusDuration = upgrades->value("bonus_duration", 0);
			ShipUpgradeRules::Clamp(result.upgrades);
		}
		if (const auto parts{ data.find("collected_part_ids") };
			parts != data.end() && parts->is_array())
		{
			for (const Json& id : *parts)
			{
				if (id.is_string() && !id.get_ref<const std::string&>().empty())
					result.collectedPartIds.push_back(id.get<std::string>());
			}
		}

        if (const auto completed{ data.find("completed_levels") };
            completed != data.end() && completed->is_array())
        {
            for (const Json& level : *completed)
            {
                if (level.is_number_integer() && level.get<int>() > 0)
                    result.completedLevels.push_back(level.get<int>());
            }
        }

        if (const auto scores{ data.find("level_best_scores") };
            scores != data.end() && scores->is_object())
        {
            for (const auto& [levelText, score] : scores->items())
            {
                try
                {
                    const int level{ std::stoi(levelText) };
                    if (level > 0 && score.is_number_integer())
                        result.levelBestScores[level] = std::max(0, score.get<int>());
                }
                catch (const std::exception&)
                {
                }
            }
        }

        return result;
    }

    bool ReplaceFile(const std::filesystem::path& temporaryPath, const std::filesystem::path& targetPath)
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
        if (error)
            return false;

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

CampaignSaveManager::CampaignSaveManager()
    : filePath(ResolveSavePath())
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

const std::filesystem::path& CampaignSaveManager::GetFilePath() const noexcept
{
    return filePath;
}

bool CampaignSaveManager::Load()
{
    std::ifstream file(filePath);
    if (!file)
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
        return PreserveCorruptSave();
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

bool CampaignSaveManager::UnlockNewContent(int availableLevels)
{
	if (!progress || availableLevels <= 0)
		return true;
	if (progress->phase != CampaignPhase::ContentComplete ||
		progress->currentLevel >= availableLevels ||
		std::ranges::find(progress->completedLevels, progress->currentLevel) ==
			progress->completedLevels.end())
	{
		return true;
	}

	const CampaignProgress previous{ *progress };
	++progress->currentLevel;
	progress->highestUnlockedLevel = std::max(
		progress->highestUnlockedLevel, progress->currentLevel);
	progress->campaignCompleted = false;
	progress->phase = CampaignPhase::Playing;
	if (Save())
		return true;
	progress = previous;
	return false;
}

bool CampaignSaveManager::Save() const
{
    if (!progress)
        return false;

    std::error_code error;
    std::filesystem::create_directories(filePath.parent_path(), error);
    if (error)
        return false;

    std::filesystem::path temporaryPath{ filePath };
    temporaryPath += ".tmp";
    {
        std::ofstream file(temporaryPath, std::ios::trunc);
        if (!file)
            return false;

        file << Serialize(progress.value()).dump(4) << '\n';
        if (!file)
            return false;
    }

    return ReplaceFile(temporaryPath, filePath);
}

bool CampaignSaveManager::DeleteSave()
{
    std::error_code error;
    const bool existed{ std::filesystem::exists(filePath, error) };
    if (error)
        return false;

    if (existed && !std::filesystem::remove(filePath, error))
        return false;
    if (error)
        return false;

    progress.reset();
    return true;
}

std::filesystem::path CampaignSaveManager::ResolveSavePath()
{
    char* localAppData{ nullptr };
    std::size_t length{ 0 };
    if (_dupenv_s(&localAppData, &length, "LOCALAPPDATA") == 0 && localAppData != nullptr)
    {
        const std::filesystem::path directory{
            std::filesystem::path(localAppData) / "Alone Bull Company" / "Until Last Asteroid"
        };
        std::free(localAppData);
        return directory / "campaign.json";
    }

    std::free(localAppData);
    return std::filesystem::current_path() / "user_data" / "campaign.json";
}

bool CampaignSaveManager::PreserveCorruptSave()
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
    if (error)
        return false;

    std::filesystem::rename(filePath, corruptPath, error);
    return !error;
}
