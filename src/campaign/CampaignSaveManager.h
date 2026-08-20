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
    [[nodiscard]] const std::filesystem::path& GetFilePath() const noexcept;

    bool Load();
    bool StartNewCampaign();
	bool UnlockNewContent(int availableLevels);
    [[nodiscard]] bool Save() const;
    bool DeleteSave();

private:
    [[nodiscard]] static std::filesystem::path ResolveSavePath();
    bool PreserveCorruptSave();

    std::optional<CampaignProgress> progress;
    std::filesystem::path filePath;
};
