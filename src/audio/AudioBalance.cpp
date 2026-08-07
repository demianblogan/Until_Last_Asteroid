#include "AudioBalance.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace
{
    using Json = nlohmann::json;

    constexpr std::array<const char*, static_cast<std::size_t>(Config::Music::Count)> MusicNames{
        "company_splash",
        "main_menu_background",
        "gameplay_theme"
    };
    constexpr std::array<const char*, static_cast<std::size_t>(Config::Sound::Count)> SoundNames{
        "character_typing",
        "interface_activation",
        "item_select",
        "item_press",
        "player_laser_shot",
        "enemy_laser_shot",
        "saucer_kamikaze_spawn",
        "saucer_shooter_spawn",
        "player_ship_explosion",
        "enemy_saucer_explosion",
        "small_meteor_explosion",
        "medium_meteor_explosion",
        "big_meteor_explosion"
    };

    template <std::size_t Size>
    void ReadVolumes(
        const Json& parent,
        const char* category,
        const std::array<const char*, Size>& names,
        std::array<float, Size>& volumes)
    {
        const auto object{ parent.find(category) };
        if (object == parent.end() || !object->is_object())
            throw std::runtime_error(std::string("Missing audio balance category: ") + category);

        for (std::size_t index{ 0u }; index < Size; ++index)
        {
            const auto value{ object->find(names[index]) };
            if (value == object->end() || !value->is_number())
                throw std::runtime_error(std::string("Missing audio balance value: ") + names[index]);

            volumes[index] = std::clamp(value->get<float>(), 0.f, 100.f);
        }
    }
}

AudioBalance::AudioBalance(const std::filesystem::path& path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Failed to load audio balance: " + path.string());

    try
    {
        const Json data{ Json::parse(file) };
        ReadVolumes(data, "music", MusicNames, musicVolumes);
        ReadVolumes(data, "sounds", SoundNames, soundVolumes);
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error(
            "Invalid audio balance file " + path.string() + ": " + exception.what());
    }
}

float AudioBalance::GetMusicVolume(Config::Music id) const noexcept
{
    return musicVolumes[static_cast<std::size_t>(id)];
}

float AudioBalance::GetSoundVolume(Config::Sound id) const noexcept
{
    return soundVolumes[static_cast<std::size_t>(id)];
}
