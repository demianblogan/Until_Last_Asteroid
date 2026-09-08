#pragma once

#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

enum class AchievementID
{
	FirstStep,
	HalfwayThere,
	CampaignComplete,
	RunSurvivor,
	HordeSurvivor,
	FullyUpgraded,
	TutorialSkipped,
	FlawlessCampaign,
	BossUntouched,

	Count
};

// Static, load-time metadata for one achievement (title, description, icon,
// unlock threshold). This does NOT track whether the player has actually
// unlocked it -- that live state lives separately in
// AchievementManager::unlockedAchievementIDs, keyed by AchievementID.
struct AchievementDefinition
{
	AchievementID id = AchievementID::FirstStep;

	// Stable string key written to the save file and used to look up
	// localized text -- unlike `id`, this survives across enum reordering.
	std::string persistentID;

	// Position in the Achievements screen's tile grid (0 = first tile, filled
	// row by row). Independent of the JSON array order or the AchievementID enum order.
	int gridPosition = 0;

	std::string title;
	std::string description;

	// Value the tracked stat must reach to unlock (seconds survived, waves
	// survived, level number reached, etc. -- meaning depends on the achievement).
	int threshold = 0;

	std::filesystem::path iconPath;
};

class AchievementManager
{
public:
	AchievementManager();

	bool LoadDefinitions(const std::filesystem::path& path);
	bool LoadProgress();
	[[nodiscard]] bool SaveProgress() const;
	[[nodiscard]] bool Unlock(AchievementID id);

	[[nodiscard]] std::optional<AchievementID> PopNotification();
	[[nodiscard]] bool IsUnlocked(AchievementID id) const noexcept;

	[[nodiscard]] const AchievementDefinition& GetDefinition(AchievementID id) const;
	[[nodiscard]] const std::vector<AchievementDefinition>& GetDefinitions() const noexcept;

private:
	[[nodiscard]] static std::filesystem::path ResolvePath();

	std::vector<AchievementDefinition> definitions;
	std::unordered_set<AchievementID> unlockedAchievementIDs;
	std::deque<AchievementID> notifications;
	std::filesystem::path filePath;
};