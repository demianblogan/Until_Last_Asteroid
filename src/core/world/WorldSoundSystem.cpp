#include "WorldSoundSystem.h"

#include "audio/AudioManager.h"

WorldSoundSystem::WorldSoundSystem(AudioManager& audioManager) noexcept
	: audio(audioManager)
{}

std::uint64_t WorldSoundSystem::AddSound(Config::Sound id, float pitch)
{
	return audio.PlaySound(id, SoundGroup::Gameplay, 100.f, pitch);
}

std::uint64_t WorldSoundSystem::AddSustainedSound(
	Config::Sound id,
	float pitch,
	float loopStartSeconds,
	float loopEndSeconds,
	float outroStartSeconds)
{
	return audio.PlaySustainedSound(
		id, SoundGroup::Gameplay, 100.f, pitch,
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
