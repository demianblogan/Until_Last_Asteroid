#include "AudioManager.h"

#include <algorithm>
#include <cmath>

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>

#include "assets/AssetStore.h"
#include "settings/SettingsManager.h"

namespace
{
	bool IsGameplayMusic(Config::Music id) noexcept
	{
		return id == Config::Music::GameplayBackground1 ||
			id == Config::Music::GameplayBackground2 ||
			id == Config::Music::GameplayBackground3;
	}
}

struct AudioManager::ActiveSound
{
	ActiveSound(std::uint64_t handle, Config::Sound id, SoundGroup group,
		float baseVolume, float basePitch,
		const sf::SoundBuffer& buffer)
        : sound(buffer)
		, handle(handle)
        , id(id)
        , group(group)
        , baseVolume(baseVolume)
		, basePitch(basePitch)
    {
    }

    sf::Sound sound;
	std::uint64_t handle;
    Config::Sound id;
    SoundGroup group;
    float baseVolume;
	float basePitch;
	bool sustained{ false };
	float loopStartSeconds{ 0.f };
	float loopEndSeconds{ 0.f };
	float outroStartSeconds{ 0.f };
};

AudioManager::AudioManager(AssetStore& assets, SettingsManager& settings)
    : assets(assets)
    , settings(settings)
    , balance("assets/data/audio_balance.json")
{
}

AudioManager::~AudioManager() = default;

void AudioManager::Update()
{
	for (const auto& activeSound : activeSounds)
	{
		if (!activeSound->sustained ||
			activeSound->sound.getStatus() != sf::Sound::Status::Playing)
		{
			continue;
		}

		const float offset{ activeSound->sound.getPlayingOffset().asSeconds() };
		if (offset >= activeSound->loopEndSeconds)
		{
			const float loopDuration{
				activeSound->loopEndSeconds - activeSound->loopStartSeconds };
			const float wrappedOffset{ activeSound->loopStartSeconds +
				std::fmod(offset - activeSound->loopStartSeconds, loopDuration) };
			activeSound->sound.setPlayingOffset(sf::seconds(wrappedOffset));
		}
	}

    std::erase_if(activeSounds, [](const auto& activeSound)
        {
            return activeSound->sound.getStatus() == sf::Sound::Status::Stopped;
        });
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
	bool looping)
{
    if (playback == SoundPlayback::Restart)
    {
        std::erase_if(activeSounds, [id, group](const auto& activeSound)
            {
                if (activeSound->id != id || activeSound->group != group)
                    return false;

                activeSound->sound.stop();
                return true;
            });
    }

	const std::uint64_t handle{ nextSoundHandle++ };
    auto activeSound{ std::make_unique<ActiveSound>(
		handle,
        id,
        group,
        baseVolume,
		pitch,
        assets.Sounds().Get(id)) };
    activeSound->sound.setAttenuation(0.f);
    activeSound->sound.setVolume(GetSoundVolume(id, baseVolume));
	activeSound->sound.setPitch(pitch * (group == SoundGroup::Gameplay ? gameplayPitch : 1.f));
	activeSound->sound.setLooping(looping);
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
	const float duration{ assets.Sounds().Get(id).getDuration().asSeconds() };
	loopStartSeconds = std::clamp(loopStartSeconds, 0.f, duration);
	loopEndSeconds = std::clamp(loopEndSeconds, loopStartSeconds + 0.01f, duration);
	outroStartSeconds = std::clamp(outroStartSeconds, loopEndSeconds, duration);

	const std::uint64_t handle{ PlaySound(
		id, group, baseVolume, pitch, SoundPlayback::AllowOverlap, false) };
	const auto found{ std::find_if(activeSounds.begin(), activeSounds.end(),
		[handle](const auto& activeSound) { return activeSound->handle == handle; }) };
	if (found != activeSounds.end())
	{
		(*found)->sustained = true;
		(*found)->loopStartSeconds = loopStartSeconds;
		(*found)->loopEndSeconds = loopEndSeconds;
		(*found)->outroStartSeconds = outroStartSeconds;
	}
	return handle;
}

void AudioManager::ReleaseSound(std::uint64_t handle)
{
	if (handle == 0u)
		return;
	std::erase_if(activeSounds, [handle](const auto& activeSound)
	{
		if (activeSound->handle != handle)
			return false;
		if (activeSound->sustained)
		{
			activeSound->sustained = false;
			activeSound->sound.setPlayingOffset(
				sf::seconds(activeSound->outroStartSeconds));
			activeSound->sound.play();
			return false;
		}
		activeSound->sound.stop();
		return true;
	});
}

void AudioManager::StopSound(std::uint64_t handle)
{
	if (handle == 0u)
		return;
	std::erase_if(activeSounds, [handle](const auto& activeSound)
	{
		if (activeSound->handle != handle)
			return false;
		activeSound->sound.stop();
		return true;
	});
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
    std::erase_if(activeSounds, [group](const auto& activeSound)
        {
            if (activeSound->group != group)
                return false;

            activeSound->sound.stop();
            return true;
        });
}

void AudioManager::SetGameplayPitch(float pitch)
{
	gameplayPitch = std::clamp(pitch, 0.1f, 2.f);
	for (const auto& activeSound : activeSounds)
	{
		if (activeSound->group == SoundGroup::Gameplay)
			activeSound->sound.setPitch(activeSound->basePitch * gameplayPitch);
	}
	for (const Config::Music id : {
		Config::Music::GameplayBackground1,
		Config::Music::GameplayBackground2,
		Config::Music::GameplayBackground3 })
	{
		assets.Music().Get(id).setPitch(gameplayPitch);
	}
}

void AudioManager::PlayGameplayMusic(
	Config::Music id, bool looping, float baseVolume)
{
	if (!IsGameplayMusic(id))
		return;
	if (activeGameplayMusic && *activeGameplayMusic != id)
		StopMusic(*activeGameplayMusic);
	activeGameplayMusic = id;
	PlayMusic(id, looping, baseVolume);
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

void AudioManager::PlayMusic(Config::Music id, bool looping, float baseVolume)
{
    sf::Music& music{ assets.Music().Get(id) };
    musicBaseVolumes[id] = baseVolume;
    music.setLooping(looping);
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
    sf::Music& music{ assets.Music().Get(id) };
    if (music.getStatus() == sf::SoundSource::Status::Paused)
        music.play();
}

bool AudioManager::IsMusicPlaying(Config::Music id) const
{
    return assets.Music().Get(id).getStatus() == sf::SoundSource::Status::Playing;
}

float AudioManager::GetMusicVolume(Config::Music id, float playbackVolume) const noexcept
{
    const AudioSettings& audio{ settings.Get().audio };
    const float resourceVolume{ balance.GetMusicVolume(id) };
    return std::clamp(
        playbackVolume * resourceVolume * audio.musicVolume / 10'000.f,
        0.f,
        100.f);
}

float AudioManager::GetSoundVolume(Config::Sound id, float playbackVolume) const noexcept
{
    const AudioSettings& audio{ settings.Get().audio };
    const float resourceVolume{ balance.GetSoundVolume(id) };
    return std::clamp(
        playbackVolume * resourceVolume * audio.soundVolume / 10'000.f,
        0.f,
        100.f);
}
