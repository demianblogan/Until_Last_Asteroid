#pragma once

#include <cstdint>

#include "utils/ConfigEnums.h"

class AudioManager;

// Owns the fixed playback conventions (SoundGroup::Gameplay, default pan) that
// every in-world sound (weapon fire, impacts, pickups) is played with, so World
// itself doesn't need to know AudioManager's lower-level parameters.
class WorldSoundSystem
{
public:
	explicit WorldSoundSystem(AudioManager& audioManager) noexcept;

	std::uint64_t AddSound(Config::Sound id, float pitch = 1.f);
	std::uint64_t AddSustainedSound(
		Config::Sound id,
		float pitch,
		float loopStartSeconds,
		float loopEndSeconds,
		float outroStartSeconds);
	void StopSound(std::uint64_t handle);

	void PauseActiveSounds();
	void ResumePausedSounds();
	void StopActiveSounds();

private:
	AudioManager& audio;
};
