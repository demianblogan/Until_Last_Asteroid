#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "utils/ConfigEnums.h"

class AssetStore;
class SettingsManager;

enum class SoundGroup
{
    UI,
    Gameplay
};

enum class SoundPlayback
{
    AllowOverlap,
    Restart
};

class AudioManager
{
public:
    AudioManager(AssetStore& assets, SettingsManager& settings);
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    void Update();
    void ApplySettings();

    void PlaySound(
        Config::Sound id,
        SoundGroup group,
        float baseVolume = 100.f,
        float pitch = 1.f,
        SoundPlayback playback = SoundPlayback::AllowOverlap);
    void PauseSounds(SoundGroup group);
    void ResumeSounds(SoundGroup group);

    void PlayMusic(Config::Music id, bool looping = true, float baseVolume = 100.f);
    void StopMusic(Config::Music id);
    void PauseMusic(Config::Music id);
    void ResumeMusic(Config::Music id);
    [[nodiscard]] bool IsMusicPlaying(Config::Music id) const;

private:
    struct ActiveSound;

    [[nodiscard]] float GetMusicVolume(float baseVolume) const noexcept;
    [[nodiscard]] float GetSoundVolume(float baseVolume) const noexcept;

    AssetStore& assets;
    SettingsManager& settings;
    std::vector<std::unique_ptr<ActiveSound>> activeSounds;
    std::unordered_map<Config::Music, float> musicBaseVolumes;
};
