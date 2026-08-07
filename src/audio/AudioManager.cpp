#include "AudioManager.h"

#include <algorithm>

#include <SFML/Audio/Music.hpp>
#include <SFML/Audio/Sound.hpp>

#include "assets/AssetStore.h"
#include "settings/SettingsManager.h"

struct AudioManager::ActiveSound
{
    ActiveSound(Config::Sound id, SoundGroup group, float baseVolume, const sf::SoundBuffer& buffer)
        : sound(buffer)
        , id(id)
        , group(group)
        , baseVolume(baseVolume)
    {
    }

    sf::Sound sound;
    Config::Sound id;
    SoundGroup group;
    float baseVolume;
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

void AudioManager::PlaySound(
    Config::Sound id,
    SoundGroup group,
    float baseVolume,
    float pitch,
    SoundPlayback playback)
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

    auto activeSound{ std::make_unique<ActiveSound>(
        id,
        group,
        baseVolume,
        assets.Sounds().Get(id)) };
    activeSound->sound.setAttenuation(0.f);
    activeSound->sound.setVolume(GetSoundVolume(id, baseVolume));
    activeSound->sound.setPitch(pitch);
    activeSound->sound.play();
    activeSounds.push_back(std::move(activeSound));
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

void AudioManager::PlayMusic(Config::Music id, bool looping, float baseVolume)
{
    sf::Music& music{ assets.Music().Get(id) };
    musicBaseVolumes[id] = baseVolume;
    music.setLooping(looping);
    music.setVolume(GetMusicVolume(id, baseVolume));
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
