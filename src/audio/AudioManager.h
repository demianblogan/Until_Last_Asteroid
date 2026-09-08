#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "audio/AudioBalance.h"
#include "utils/ConfigEnums.h"

class Assets;
class SettingsManager;

// Lets sounds be paused/resumed/stopped (PauseSounds/ResumeSounds/StopSounds)
// and pitch-shifted (SetGameplayAudioPitch) as a group instead of one at a
// time -- e.g. pausing gameplay sounds on the pause menu without silencing
// its own UI click sound, or applying the time-slowdown pitch effect only
// to the game world's own sounds/music, not the interface.
enum class SoundGroup
{
	UI,
	Gameplay
};

// What happens when PlaySound() is called for an id that already has an
// active instance playing.
enum class SoundPlayback
{
	// Start a new, independent instance alongside the one already playing
	// (e.g. rapid-fire impact sounds that should layer/overlap).
	AllowOverlap,

	// Stop the existing instance of this same id+group first, then start the
	// new one (e.g. a UI click sound that would sound muddy if it overlapped
	// itself on rapid repeat presses).
	StopPrevious
};

class AudioManager
{
public:
	AudioManager(Assets& assets, SettingsManager& settings);
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
		bool isLooping = false);

	// Plays id once through from the start, then loops just the
	// [loopStartSeconds, loopEndSeconds) segment indefinitely -- for sounds
	// with a one-time attack followed by a sustainable body (e.g. the
	// player's laser beam: a short wind-up, then a seamless hum for as long
	// as the fire button stays held). outroStartSeconds is reserved for a
	// release tail and not applied yet.
	std::uint64_t PlaySustainedSound(
		Config::Sound id,
		SoundGroup group,
		float baseVolume,
		float pitch,
		float loopStartSeconds,
		float loopEndSeconds,
		float outroStartSeconds);

	void StopSound(std::uint64_t handle);

	void PauseSounds(SoundGroup group);
	void ResumeSounds(SoundGroup group);
	void StopSounds(SoundGroup group);

	// Global pitch multiplier applied to every Gameplay-group sound and
	// gameplay music track at once -- the audio side of the time-slowdown
	// pickup (world time dilates, so its sounds/music pitch down to match,
	// like a slowed-down record). UI sounds are unaffected, since the
	// interface doesn't slow down with the game world. Clamped to [0.1, 2].
	void SetGameplayAudioPitch(float pitch);

	void PlayGameplayMusic(Config::Music id, bool isLooping = true, float baseVolume = 100.f);
	void StopGameplayMusic();
	void PauseGameplayMusic();
	void ResumeGameplayMusic();

	[[nodiscard]] bool IsGameplayMusicPlaying() const;

	void PlayMusic(Config::Music id, bool isLooping = true, float baseVolume = 100.f);
	void StopMusic(Config::Music id);
	void PauseMusic(Config::Music id);
	void ResumeMusic(Config::Music id);
	[[nodiscard]] bool IsMusicPlaying(Config::Music id) const;

private:
	struct ActiveSound;

	[[nodiscard]] float GetMusicVolume(Config::Music id, float playbackVolume) const noexcept;
	[[nodiscard]] float GetSoundVolume(Config::Sound id, float playbackVolume) const noexcept;

	Assets& assets;
	SettingsManager& settings;
	AudioBalance balance;
	std::vector<std::unique_ptr<ActiveSound>> activeSounds;
	std::unordered_map<Config::Music, float> musicBaseVolumes;
	std::optional<Config::Music> activeGameplayMusic;
	float gameplayPitch = 1.f;
	std::uint64_t nextSoundHandle = 1u;
};