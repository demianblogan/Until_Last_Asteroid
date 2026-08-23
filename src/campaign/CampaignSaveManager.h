#pragma once

#include <filesystem>
#include <optional>

#include "campaign/CampaignProgress.h"

class CampaignSaveManager
{
public:
	CampaignSaveManager();

	[[nodiscard]] bool HasSave() const noexcept;
	[[nodiscard]] const CampaignProgress* GetProgress() const noexcept;
	[[nodiscard]] CampaignProgress* EditProgress() noexcept;

	bool Load();
	bool StartNewCampaign();
	void UnlockNewLevel(int availableLevels);

	[[nodiscard]] bool Save() const;
	bool DeleteSave();

private:
	[[nodiscard]] static std::filesystem::path ResolveSavePath();

	std::optional<CampaignProgress> progress;
	std::filesystem::path saveFilePath;
};