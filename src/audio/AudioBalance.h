#pragma once

#include <array>
#include <filesystem>

#include "utils/ConfigEnums.h"

// Per-sound and per-track relative volume, loaded once from
// assets/data/audio_balance.json (design-time mixing data, not a player
// setting). Values are on a 0-100 scale, matching SFML's own
// sf::Sound/sf::Music::setVolume() range.
//
// This is only one of three factors AudioManager multiplies together for
// the volume it actually plays a sound/track at: this fixed per-resource
// balance, the caller's own per-instance playback volume, and the
// player's music/sound sliders from settings.
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
