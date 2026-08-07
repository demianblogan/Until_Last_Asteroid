#pragma once

#include <filesystem>

#include "settings/GameSettings.h"

class SettingsManager
{
public:
    SettingsManager();

    [[nodiscard]] const GameSettings& Get() const noexcept;
    [[nodiscard]] const GameSettings& GetDefaults() const noexcept;
    [[nodiscard]] GameSettings& Edit() noexcept;
    [[nodiscard]] const std::filesystem::path& GetFilePath() const noexcept;

    bool Load();
    [[nodiscard]] bool Save() const;
    bool ResetToDefaults();

private:
    [[nodiscard]] static GameSettings CreateDefaults();
    [[nodiscard]] static std::filesystem::path ResolveSettingsPath();

    GameSettings defaults;
    GameSettings settings;
    std::filesystem::path filePath;
};
