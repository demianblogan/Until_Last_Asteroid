#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "audio/AudioBalance.h"
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

    std::uint64_t PlaySound(
        Config::Sound id,
        SoundGroup group,
        float baseVolume = 100.f,
        float pitch = 1.f,
		SoundPlayback playback = SoundPlayback::AllowOverlap,
		bool looping = false);
	std::uint64_t PlaySustainedSound(
		Config::Sound id,
		SoundGroup group,
		float baseVolume,
		float pitch,
		float loopStartSeconds,
		float loopEndSeconds,
		float outroStartSeconds);
	void ReleaseSound(std::uint64_t handle);
    void StopSound(std::uint64_t handle);
    void PauseSounds(SoundGroup group);
    void ResumeSounds(SoundGroup group);
    void StopSounds(SoundGroup group);
	void SetGameplayPitch(float pitch);
	void PlayGameplayMusic(Config::Music id, bool looping = true, float baseVolume = 100.f);
	void StopGameplayMusic();
	void PauseGameplayMusic();
	void ResumeGameplayMusic();
	[[nodiscard]] bool IsGameplayMusicPlaying() const;

    void PlayMusic(Config::Music id, bool looping = true, float baseVolume = 100.f);
    void StopMusic(Config::Music id);
    void PauseMusic(Config::Music id);
    void ResumeMusic(Config::Music id);
    [[nodiscard]] bool IsMusicPlaying(Config::Music id) const;

private:
    struct ActiveSound;

    [[nodiscard]] float GetMusicVolume(Config::Music id, float playbackVolume) const noexcept;
    [[nodiscard]] float GetSoundVolume(Config::Sound id, float playbackVolume) const noexcept;

    AssetStore& assets;
    SettingsManager& settings;
    AudioBalance balance;
    std::vector<std::unique_ptr<ActiveSound>> activeSounds;
	std::unordered_map<Config::Music, float> musicBaseVolumes;
	std::optional<Config::Music> activeGameplayMusic;
	float gameplayPitch{ 1.f };
	std::uint64_t nextSoundHandle{ 1u };
};
