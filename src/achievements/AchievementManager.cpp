#include "AchievementManager.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>
#include <system_error>

#include <nlohmann/json.hpp>

#include "utils/AppDataPath.h"
#include "utils/SafeFileWrite.h"

namespace
{
	using Json = nlohmann::json;
	using SafeFileWrite::HasFailed;

	// Written into every saved progress file and checked on load. Bump this
	// if the on-disk schema of the player's save (not the shipped
	// achievement_definitions.json) ever changes in an incompatible way --
	// LoadProgress() will then refuse to interpret an older-format file
	// instead of misreading fields that no longer mean the same thing.
	constexpr int ProgressFormatVersion = 1;
	constexpr int JSONIndentWidth = 4;

	constexpr std::array<const char*, static_cast<std::size_t>(AchievementID::Count)> IDNames =
	{
		"first_step",
		"halfway_there",
		"campaign_complete",
		"run_survivor",
		"horde_survivor",
		"fully_upgraded",
		"tutorial_skipped",
		"flawless_campaign",
		"boss_untouched"
	};

	std::string_view SerializeID(AchievementID id)
	{
		const std::size_t index = static_cast<std::size_t>(id);

		if (index >= IDNames.size())
			throw std::runtime_error("invalid achievement id");

		return IDNames[index];
	}

	AchievementID DeserializeID(std::string_view text)
	{
		const auto found = std::ranges::find(IDNames, text);

		if (found == IDNames.end())
			throw std::runtime_error("unknown achievement id");

		return static_cast<AchievementID>(std::distance(IDNames.begin(), found));
	}
}

AchievementManager::AchievementManager()
	: filePath(ResolvePath())
{}

bool AchievementManager::LoadDefinitions(const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return false;

	try
	{
		const Json root = Json::parse(file);
		if (!root.is_object() || root.value("format_version", 0) != 1)
			return false;

		const Json& items = root.at("achievements");
		if (!items.is_array() || items.size() != IDNames.size())
			return false;

		std::vector<AchievementDefinition> parsedDefinitions;
		std::unordered_set<AchievementID> seenIDs;
		std::unordered_set<int> seenOrders;

		parsedDefinitions.reserve(items.size());

		for (const Json& item : items)
		{
			AchievementDefinition definition;
			definition.persistentID = item.at("persistent_id").get<std::string>();
			definition.id = DeserializeID(definition.persistentID);
			definition.gridPosition = item.at("grid_position").get<int>();
			definition.title = item.at("title").get<std::string>();
			definition.description = item.at("description").get<std::string>();
			definition.threshold = item.at("threshold").get<int>();
			definition.iconPath = item.at("icon").get<std::string>();

			if (definition.gridPosition < 0 || definition.gridPosition >= static_cast<int>(IDNames.size()) ||
				definition.title.empty() || definition.description.empty() ||
				definition.threshold < 0 || definition.iconPath.empty() ||
				!seenIDs.insert(definition.id).second ||
				!seenOrders.insert(definition.gridPosition).second)
			{
				return false;
			}

			parsedDefinitions.push_back(std::move(definition));
		}

		auto byGridPosition = [](const AchievementDefinition& a, const AchievementDefinition& b)
			{
				return a.gridPosition < b.gridPosition;
			};

		std::ranges::sort(parsedDefinitions, byGridPosition);

		definitions = std::move(parsedDefinitions);

		return true;
	}
	catch (const std::exception&)
	{
		return false;
	}
}

bool AchievementManager::LoadProgress()
{
	std::ifstream file(filePath);
	if (!file.is_open())
	{
		unlockedAchievementIDs.clear();
		return true;
	}
	try
	{
		const Json root = Json::parse(file);
		if (!root.is_object() || root.value("format_version", 0) != ProgressFormatVersion)
			throw std::runtime_error("unsupported achievements format");

		std::unordered_set<AchievementID> parsedUnlockedIDs;

		const Json& items = root.at("unlocked");
		if (!items.is_array())
			throw std::runtime_error("unlocked must be an array");

		for (const Json& item : items)
			if (item.is_string())
				parsedUnlockedIDs.insert(DeserializeID(item.get<std::string>()));

		unlockedAchievementIDs = std::move(parsedUnlockedIDs);

		return true;
	}
	catch (const std::exception&)
	{
		unlockedAchievementIDs.clear();

		return SafeFileWrite::PreserveCorruptFile(filePath);
	}
}

bool AchievementManager::SaveProgress() const
{
	std::error_code error;

	std::filesystem::create_directories(filePath.parent_path(), error);
	if (HasFailed(error))
		return false;

	std::vector<std::string> IDs;
	IDs.reserve(unlockedAchievementIDs.size());

	for (const AchievementID ID : unlockedAchievementIDs)
		IDs.emplace_back(SerializeID(ID));

	std::ranges::sort(IDs);

	const Json root =
	{
		{ "format_version", ProgressFormatVersion },
		{ "unlocked", IDs }
	};

	std::filesystem::path temporaryPath = filePath;
	temporaryPath += ".tmp";

	{
		std::ofstream file(temporaryPath, std::ios::trunc);
		if (!file.is_open())
			return false;

		// Broader stream-state check on purpose (not is_open()): this needs
		// to catch a failed write too (disk full, permissions lost mid-write),
		// not just "did the file open".
		file << root.dump(JSONIndentWidth) << '\n';
		if (!file)
			return false;
	}

	return SafeFileWrite::ReplaceFileAtomically(temporaryPath, filePath);
}

bool AchievementManager::Unlock(AchievementID id)
{
	if (id == AchievementID::Count || unlockedAchievementIDs.contains(id))
		return false;

	unlockedAchievementIDs.insert(id);

	if (SaveProgress())
	{
		notifications.push_back(id);
		return true;
	}

	unlockedAchievementIDs.erase(id);

	return false;
}

std::optional<AchievementID> AchievementManager::PopNotification()
{
	if (notifications.empty())
		return std::nullopt;

	const AchievementID id = notifications.front();
	notifications.pop_front();

	return id;
}

bool AchievementManager::IsUnlocked(AchievementID id) const noexcept
{
	return unlockedAchievementIDs.contains(id);
}

const AchievementDefinition& AchievementManager::GetDefinition(AchievementID id) const
{
	const auto found = std::ranges::find(definitions, id, &AchievementDefinition::id);
	if (found == definitions.end())
		throw std::runtime_error("achievement definition not loaded");

	return *found;
}

const std::vector<AchievementDefinition>& AchievementManager::GetDefinitions() const noexcept
{
	return definitions;
}

std::filesystem::path AchievementManager::ResolvePath()
{
	return AppDataPath::Resolve("achievements.json");
}

