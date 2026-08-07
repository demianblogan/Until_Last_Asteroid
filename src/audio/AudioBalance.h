#pragma once

#include <array>
#include <filesystem>

#include "utils/ConfigEnums.h"

class AudioBalance
{
public:
    explicit AudioBalance(const std::filesystem::path& path);

    [[nodiscard]] float GetMusicVolume(Config::Music id) const noexcept;
    [[nodiscard]] float GetSoundVolume(Config::Sound id) const noexcept;

private:
    std::array<float, static_cast<std::size_t>(Config::Music::Count)> musicVolumes;
    std::array<float, static_cast<std::size_t>(Config::Sound::Count)> soundVolumes;
};
