#include "WorldSoundSystem.h"

#include "audio/AudioManager.h"

namespace
{
	// Every in-world sound plays at full base volume -- AudioManager's own
	// per-group/per-setting attenuation handles the rest.
	constexpr float FullBaseVolume = 100.f;
}

WorldSoundSystem::WorldSoundSystem(AudioManager& audioManager) noexcept
	: audio(audioManager)
{}

std::uint64_t WorldSoundSystem::AddSound(Config::Sound id, float pitch)
{
	return audio.PlaySound(id, SoundGroup::Gameplay, FullBaseVolume, pitch);
}

std::uint64_t WorldSoundSystem::AddSustainedSound(
	Config::Sound id,
	float pitch,
	float loopStartSeconds,
	float loopEndSeconds,
	float outroStartSeconds)
{
	return audio.PlaySustainedSound(
		id, SoundGroup::Gameplay, FullBaseVolume, pitch,
		loopStartSeconds, loopEndSeconds, outroStartSeconds);
}

void WorldSoundSystem::StopSound(std::uint64_t handle)
{
	audio.StopSound(handle);
}

void WorldSoundSystem::PauseActiveSounds()
{
	audio.PauseSounds(SoundGroup::Gameplay);
}

void WorldSoundSystem::ResumePausedSounds()
{
	audio.ResumeSounds(SoundGroup::Gameplay);
}

void WorldSoundSystem::StopActiveSounds()
{
	audio.StopSounds(SoundGroup::Gameplay);
}
