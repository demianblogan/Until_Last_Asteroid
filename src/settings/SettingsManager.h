#pragma once

#include <filesystem>

#include "settings/GameSettings.h"

class SettingsManager
{
public:
    SettingsManager();

    [[nodiscard]] const GameSettings& GetSettings() const noexcept;
    [[nodiscard]] const GameSettings& GetDefaults() const noexcept;
    [[nodiscard]] GameSettings& EditSettings() noexcept;
    bool LoadSettings();
    [[nodiscard]] bool SaveSettings() const;

    bool ResetSettingsToDefaults();

private:
    [[nodiscard]] static GameSettings CreateDefaults();
    [[nodiscard]] static std::filesystem::path ResolveSettingsPath();

    GameSettings defaults;
    GameSettings settings;
    std::filesystem::path settingsFilePath;
};
