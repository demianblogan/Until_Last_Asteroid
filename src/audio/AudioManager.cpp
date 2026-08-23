#include "AudioManager.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>

#include "assets/Assets.h"
#include "settings/SettingsManager.h"

namespace
{
	// The music tracks that count as "gameplay music" -- the only ones
	// PlayGameplayMusic() will accept and SetGameplayAudioPitch() pitches
	// down together for the time-slowdown effect. Everything else (menu
	// themes, the company splash) is played through PlayMusic() directly
	// and never pitch-shifted.
	constexpr std::array GameplayMusicIDs =
	{
		Config::Music::GameplayBackground1,
		Config::Music::GameplayBackground2,
		Config::Music::GameplayBackground3,
		Config::Music::BossFight,
		Config::Music::CampaignVictory
	};

	bool IsGameplayMusic(Config::Music id) noexcept
	{
		return std::ranges::find(GameplayMusicIDs, id) != GameplayMusicIDs.end();
	}
}

struct AudioManager::ActiveSound
{
	ActiveSound(
		std::uint64_t handle,
		Config::Sound id,
		SoundGroup group,
		float baseVolume,
		float basePitch,
		const sf::SoundBuffer& buffer)
		: sound(buffer)
		, handle(handle)
		, id(id)
		, group(group)
		, baseVolume(baseVolume)
		, basePitch(basePitch)
	{}

	sf::Sound sound;
	std::uint64_t handle;
	Config::Sound id;
	SoundGroup group;
	float baseVolume;
	float basePitch;

	bool isSustained = false;
	float loopStartSeconds = 0.f;
	float loopEndSeconds = 0.f;
	float outroStartSeconds = 0.f;
};

AudioManager::AudioManager(Assets& assets, SettingsManager& settings)
	: assets(assets)
	, settings(settings)
	, balance("assets/data/audio_balance.json")
{}

// Defined here (not inline in the header) on purpose. AudioManager.h only
// forward-declares `struct ActiveSound;` -- nobody outside this .cpp file
// knows what it actually contains. Destroying `activeSounds`
// (a vector of unique_ptr<ActiveSound>) requires knowing ActiveSound's real
// definition, which is only visible here, below, after the full `struct
// AudioManager::ActiveSound { ... }`. If this destructor were `= default`
// in the header instead, any other .cpp that merely includes
// AudioManager.h would fail to compile trying to generate that same
// destruction code without ever having seen what ActiveSound is.
AudioManager::~AudioManager() = default;

void AudioManager::Update()
{
	for (const auto& activeSound : activeSounds)
	{
		if (!activeSound->isSustained)
			continue;

		// SFML has no concept of "loop only this sub-range" -- setLooping()
		// can only repeat the whole clip. So this sound is played with
		// isLooping=false (see PlaySustainedSound) and manually restarted
		// each time SFML reports it as fully Stopped, seeking to
		// loopStartSeconds instead of the clip's true beginning. This is
		// what lets the one-time attack (0 -> loopStartSeconds) play once
		// while only [loopStartSeconds, loopEndSeconds) repeats after it.
		if (activeSound->sound.getStatus() == sf::Sound::Status::Stopped)
		{
			activeSound->sound.setPlayingOffset(sf::seconds(activeSound->loopStartSeconds));
			activeSound->sound.play();
			continue;
		}

		if (activeSound->sound.getStatus() != sf::Sound::Status::Playing)
			continue;

		// Mid-playback, catch the moment playback crosses loopEndSeconds and
		// seek back into the loop region instead of waiting for SFML to
		// reach the true end of the clip and report Stopped (which would
		// have to go through the restart branch above and could lose a
		// fraction of a second at the seam, audible as a stutter/click on a
		// tight loop). std::fmod wraps any overshoot back into the loop
		// region rather than always resetting to exactly loopStartSeconds,
		// so the loop stays seamless even if this Update() call landed a
		// little late.
		const float offset = activeSound->sound.getPlayingOffset().asSeconds();
		if (offset >= activeSound->loopEndSeconds)
		{
			const float loopDuration = activeSound->loopEndSeconds - activeSound->loopStartSeconds;

			const float wrappedOffset = activeSound->loopStartSeconds +
				std::fmod(offset - activeSound->loopStartSeconds, loopDuration);

			activeSound->sound.setPlayingOffset(sf::seconds(wrappedOffset));
		}
	}

	auto predicate = [](const auto& activeSound)
		{
			return !activeSound->isSustained &&
				activeSound->sound.getStatus() == sf::Sound::Status::Stopped;
		};

	std::erase_if(activeSounds, predicate);
}

void AudioManager::ApplySettings()
{
	for (const auto& activeSound : activeSounds)
		activeSound->sound.setVolume(GetSoundVolume(activeSound->id, activeSound->baseVolume));

	for (const auto& [id, baseVolume] : musicBaseVolumes)
		assets.Music().Get(id).setVolume(GetMusicVolume(id, baseVolume));
}

std::uint64_t AudioManager::PlaySound(
	Config::Sound id,
	SoundGroup group,
	float baseVolume,
	float pitch,
	SoundPlayback playback,
	bool isLooping)
{
	if (playback == SoundPlayback::StopPrevious)
	{
		auto predicate = [id, group](const auto& activeSound)
			{
				if (activeSound->id != id || activeSound->group != group)
					return false;

				activeSound->sound.stop();
				return true;
			};

		std::erase_if(activeSounds, predicate);
	}

	const std::uint64_t handle = nextSoundHandle++;

	auto activeSound =
		std::make_unique<ActiveSound>(handle, id, group, baseVolume, pitch, assets.Sounds().Get(id));

	activeSound->sound.setAttenuation(0.f);
	activeSound->sound.setVolume(GetSoundVolume(id, baseVolume));
	activeSound->sound.setPitch(pitch * (group == SoundGroup::Gameplay ? gameplayPitch : 1.f));
	activeSound->sound.setLooping(isLooping);
	activeSound->sound.play();

	activeSounds.push_back(std::move(activeSound));

	return handle;
}

std::uint64_t AudioManager::PlaySustainedSound(
	Config::Sound id,
	SoundGroup group,
	float baseVolume,
	float pitch,
	float loopStartSeconds,
	float loopEndSeconds,
	float outroStartSeconds)
{
	const float duration = assets.Sounds().Get(id).getDuration().asSeconds();

	// Defensive clamping against bad caller-supplied timestamps, keeping the
	// three markers in the only order that makes sense --
	// 0 <= loopStartSeconds < loopEndSeconds <= outroStartSeconds <= duration
	// -- so the loop above always has a positive, in-bounds loop region to
	// work with instead of, say, dividing by a zero/negative loopDuration.
	loopStartSeconds = std::clamp(loopStartSeconds, 0.f, duration);
	loopEndSeconds = std::clamp(loopEndSeconds, loopStartSeconds + 0.01f, duration);
	outroStartSeconds = std::clamp(outroStartSeconds, loopEndSeconds, duration);

	const std::uint64_t handle = PlaySound(id, group, baseVolume, pitch, SoundPlayback::AllowOverlap, false);

	auto predicate = [handle](const auto& activeSound)
		{
			return activeSound->handle == handle;
		};

	const auto found = std::find_if(activeSounds.begin(), activeSounds.end(), predicate);
	if (found != activeSounds.end())
	{
		(*found)->isSustained = true;
		(*found)->loopStartSeconds = loopStartSeconds;
		(*found)->loopEndSeconds = loopEndSeconds;
		(*found)->outroStartSeconds = outroStartSeconds;
	}

	return handle;
}

void AudioManager::StopSound(std::uint64_t handle)
{
	if (handle == 0u)
		return;

	auto predicate = [handle](const auto& activeSound)
		{
			if (activeSound->handle != handle)
				return false;

			activeSound->sound.stop();
			return true;
		};

	std::erase_if(activeSounds, predicate);
}

void AudioManager::PauseSounds(SoundGroup group)
{
	for (const auto& activeSound : activeSounds)
	{
		if (activeSound->group == group &&
			activeSound->sound.getStatus() == sf::Sound::Status::Playing)
		{
			activeSound->sound.pause();
		}
	}
}

void AudioManager::ResumeSounds(SoundGroup group)
{
	for (const auto& activeSound : activeSounds)
	{
		if (activeSound->group == group &&
			activeSound->sound.getStatus() == sf::Sound::Status::Paused)
		{
			activeSound->sound.play();
		}
	}
}

void AudioManager::StopSounds(SoundGroup group)
{
	auto predicate = [group](const auto& activeSound)
		{
			if (activeSound->group != group)
				return false;

			activeSound->sound.stop();
			return true;
		};

	std::erase_if(activeSounds, predicate);
}

void AudioManager::SetGameplayAudioPitch(float pitch)
{
	// Defensive bounds, not a design target -- the only real caller today
	// (the time-slowdown pickup) only ever lerps between 1 (normal) and its
	// configured slow-down pitch, which is well above 0. The floor keeps
	// pitch from ever hitting zero or negative (SFML requires a positive
	// pitch), and the ceiling caps how fast a future speed-up effect could
	// ever push audio, in case one is added later.
	gameplayPitch = std::clamp(pitch, 0.1f, 2.f);

	for (const auto& activeSound : activeSounds)
	{
		if (activeSound->group == SoundGroup::Gameplay)
			activeSound->sound.setPitch(activeSound->basePitch * gameplayPitch);
	}

	for (const Config::Music id : GameplayMusicIDs)
		assets.Music().Get(id).setPitch(gameplayPitch);
}

void AudioManager::PlayGameplayMusic(Config::Music id, bool isLooping, float baseVolume)
{
	if (!IsGameplayMusic(id))
		return;

	if (activeGameplayMusic && *activeGameplayMusic != id)
		StopMusic(*activeGameplayMusic);

	activeGameplayMusic = id;

	PlayMusic(id, isLooping, baseVolume);
}

void AudioManager::StopGameplayMusic()
{
	if (!activeGameplayMusic)
		return;
	StopMusic(*activeGameplayMusic);

	activeGameplayMusic.reset();
}

void AudioManager::PauseGameplayMusic()
{
	if (activeGameplayMusic)
		PauseMusic(*activeGameplayMusic);
}

void AudioManager::ResumeGameplayMusic()
{
	if (activeGameplayMusic)
		ResumeMusic(*activeGameplayMusic);
}

bool AudioManager::IsGameplayMusicPlaying() const
{
	return activeGameplayMusic && IsMusicPlaying(*activeGameplayMusic);
}

void AudioManager::PlayMusic(Config::Music id, bool isLooping, float baseVolume)
{
	sf::Music& music = assets.Music().Get(id);

	musicBaseVolumes[id] = baseVolume;
	music.setLooping(isLooping);
	music.setVolume(GetMusicVolume(id, baseVolume));
	music.setPitch(IsGameplayMusic(id) ? gameplayPitch : 1.f);

	if (music.getStatus() != sf::SoundSource::Status::Playing)
		music.play();
}

void AudioManager::StopMusic(Config::Music id)
{
	assets.Music().Get(id).stop();
}

void AudioManager::PauseMusic(Config::Music id)
{
	assets.Music().Get(id).pause();
}

void AudioManager::ResumeMusic(Config::Music id)
{
	sf::Music& music = assets.Music().Get(id);

	if (music.getStatus() == sf::SoundSource::Status::Paused)
		music.play();
}

bool AudioManager::IsMusicPlaying(Config::Music id) const
{
	return assets.Music().Get(id).getStatus() == sf::SoundSource::Status::Playing;
}

float AudioManager::GetMusicVolume(Config::Music id, float playbackVolume) const noexcept
{
	const AudioSettings& audio = settings.GetSettings().audio;
	const float resourceVolume = balance.GetMusicVolume(id);

	// Three factors are combined here, and all three are on a 0-100 scale
	// (playbackVolume, the AudioBalance value, and the player's settings
	// slider) -- not 0-1. Multiplying three 0-100 values together lands on a
	// 0-1,000,000 scale, so "... / 10'000" (== 100*100) brings it back down to 0-100:
	// exactly two of the three factors are being treated as a 0-1 fraction
	// (divide each by 100) applied on top of the third, which stays on its
	// original 0-100 scale.
	return std::clamp(playbackVolume * resourceVolume * audio.musicVolume / 10'000.f, 0.f, 100.f);
}

float AudioManager::GetSoundVolume(Config::Sound id, float playbackVolume) const noexcept
{
	const AudioSettings& audio = settings.GetSettings().audio;
	const float resourceVolume = balance.GetSoundVolume(id);

	// Three factors are combined here, and all three are on a 0-100 scale
	// (playbackVolume, the AudioBalance value, and the player's settings
	// slider) -- not 0-1. Multiplying three 0-100 values together lands on a
	// 0-1,000,000 scale, so "... / 10'000" (== 100*100) brings it back down to 0-100:
	// exactly two of the three factors are being treated as a 0-1 fraction
	// (divide each by 100) applied on top of the third, which stays on its
	// original 0-100 scale.
	return std::clamp(playbackVolume * resourceVolume * audio.soundVolume / 10'000.f, 0.f, 100.f);
}