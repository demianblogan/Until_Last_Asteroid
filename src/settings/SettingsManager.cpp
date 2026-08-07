#include "SettingsManager.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>
#include <SFML/Window/VideoMode.hpp>

namespace
{
    using Json = nlohmann::json;

    constexpr std::array<unsigned int, 7> SupportedFrameRateLimits{
        0u, 30u, 60u, 120u, 144u, 240u, 360u
    };

    template <typename Value>
    Value ReadValue(const Json& object, const char* key, Value fallback)
    {
        const auto value{ object.find(key) };
        if (value == object.end())
            return fallback;

        try
        {
            return value->get<Value>();
        }
        catch (const Json::exception&)
        {
            return fallback;
        }
    }

    std::string ToString(WindowMode mode)
    {
        switch (mode)
        {
        case WindowMode::Fullscreen:
            return "fullscreen";
        case WindowMode::Windowed:
            return "windowed";
        case WindowMode::Borderless:
            return "borderless";
        }

        return "fullscreen";
    }

    WindowMode ParseWindowMode(const std::string& value, WindowMode fallback)
    {
        if (value == "fullscreen")
            return WindowMode::Fullscreen;
        if (value == "windowed")
            return WindowMode::Windowed;
        if (value == "borderless")
            return WindowMode::Borderless;

        return fallback;
    }

    std::string ToString(InputDevice device)
    {
        return device == InputDevice::Mouse ? "mouse" : "keyboard";
    }

    InputDevice ParseInputDevice(const std::string& value, InputDevice fallback)
    {
        if (value == "keyboard")
            return InputDevice::Keyboard;
        if (value == "mouse")
            return InputDevice::Mouse;

        return fallback;
    }

    Json SerializeBinding(const ControlBinding& binding)
    {
        return {
            { "device", ToString(binding.device) },
            { "code", binding.code }
        };
    }

    ControlBinding DeserializeBinding(const Json& object, const ControlBinding& fallback)
    {
        if (!object.is_object())
            return fallback;

        ControlBinding result{ fallback };
        result.device = ParseInputDevice(
            ReadValue(object, "device", ToString(fallback.device)),
            fallback.device);
        result.code = ReadValue(object, "code", fallback.code);

        const bool validKeyboard{
            result.device == InputDevice::Keyboard &&
            result.code >= 0 &&
            result.code < static_cast<int>(sf::Keyboard::KeyCount)
        };
        const bool validMouse{
            result.device == InputDevice::Mouse &&
            result.code >= 0 &&
            result.code < static_cast<int>(sf::Mouse::ButtonCount)
        };

        return validKeyboard || validMouse ? result : fallback;
    }

    Json Serialize(const GameSettings& settings)
    {
        return {
            { "version", GameSettings::FORMAT_VERSION },
            {
                "graphics",
                {
                    { "width", settings.graphics.resolution.x },
                    { "height", settings.graphics.resolution.y },
                    { "windowMode", ToString(settings.graphics.windowMode) },
                    { "showFps", settings.graphics.showFps },
                    { "verticalSynchronization", settings.graphics.verticalSync },
                    { "frameRateLimit", settings.graphics.frameRateLimit }
                }
            },
            {
                "audio",
                {
                    { "musicMaster", settings.audio.musicVolume },
                    { "soundsMaster", settings.audio.soundVolume }
                }
            },
            {
                "controls",
                {
                    { "moveUp", SerializeBinding(settings.controls.moveUp) },
                    { "moveDown", SerializeBinding(settings.controls.moveDown) },
                    { "moveLeft", SerializeBinding(settings.controls.moveLeft) },
                    { "moveRight", SerializeBinding(settings.controls.moveRight) },
                    { "fire", SerializeBinding(settings.controls.fire) }
                }
            }
        };
    }

    GameSettings Deserialize(const Json& data, const GameSettings& defaults)
    {
        GameSettings result{ defaults };

        if (const auto graphics{ data.find("graphics") };
            graphics != data.end() && graphics->is_object())
        {
            const unsigned int width{ ReadValue(*graphics, "width", result.graphics.resolution.x) };
            const unsigned int height{ ReadValue(*graphics, "height", result.graphics.resolution.y) };
            if (width >= 640u && height >= 480u && width <= 16384u && height <= 16384u)
                result.graphics.resolution = { width, height };

            result.graphics.windowMode = ParseWindowMode(
                ReadValue(*graphics, "windowMode", ToString(result.graphics.windowMode)),
                result.graphics.windowMode);
            result.graphics.showFps = ReadValue(*graphics, "showFps", result.graphics.showFps);
            result.graphics.verticalSync = ReadValue(
                *graphics,
                "verticalSynchronization",
                result.graphics.verticalSync);

            const unsigned int frameRateLimit{
                ReadValue(*graphics, "frameRateLimit", result.graphics.frameRateLimit)
            };
            if (std::ranges::find(SupportedFrameRateLimits, frameRateLimit) != SupportedFrameRateLimits.end())
                result.graphics.frameRateLimit = frameRateLimit;
        }

        if (const auto audio{ data.find("audio") };
            audio != data.end() && audio->is_object())
        {
            const float legacyMusic{ ReadValue(*audio, "music", result.audio.musicVolume) };
            const float legacySounds{ ReadValue(*audio, "sounds", result.audio.soundVolume) };
            result.audio.musicVolume = std::clamp(
                ReadValue(*audio, "musicMaster", legacyMusic),
                0.f,
                100.f);
            result.audio.soundVolume = std::clamp(
                ReadValue(*audio, "soundsMaster", legacySounds),
                0.f,
                100.f);
        }

        if (const auto controls{ data.find("controls") };
            controls != data.end() && controls->is_object())
        {
            const auto readBinding{ [&controls](const char* key, const ControlBinding& fallback)
                {
                    const auto binding{ controls->find(key) };
                    return binding == controls->end()
                        ? fallback
                        : DeserializeBinding(*binding, fallback);
                } };

            result.controls.moveUp = readBinding("moveUp", result.controls.moveUp);
            result.controls.moveDown = readBinding("moveDown", result.controls.moveDown);
            result.controls.moveLeft = readBinding("moveLeft", result.controls.moveLeft);
            result.controls.moveRight = readBinding("moveRight", result.controls.moveRight);
            result.controls.fire = readBinding("fire", result.controls.fire);
        }

        return result;
    }

    bool ReplaceFile(const std::filesystem::path& temporaryPath, const std::filesystem::path& targetPath)
    {
        std::error_code error;
        if (!std::filesystem::exists(targetPath, error))
        {
            std::filesystem::rename(temporaryPath, targetPath, error);
            return !error;
        }

        std::filesystem::path backupPath{ targetPath };
        backupPath += ".bak";
        std::filesystem::remove(backupPath, error);
        error.clear();

        std::filesystem::rename(targetPath, backupPath, error);
        if (error)
            return false;

        std::filesystem::rename(temporaryPath, targetPath, error);
        if (error)
        {
            std::error_code restoreError;
            std::filesystem::rename(backupPath, targetPath, restoreError);
            return false;
        }

        std::filesystem::remove(backupPath, error);
        return true;
    }
}

SettingsManager::SettingsManager()
    : defaults(CreateDefaults())
    , settings(defaults)
    , filePath(ResolveSettingsPath())
{
    Load();
}

const GameSettings& SettingsManager::Get() const noexcept
{
    return settings;
}

const GameSettings& SettingsManager::GetDefaults() const noexcept
{
    return defaults;
}

GameSettings& SettingsManager::Edit() noexcept
{
    return settings;
}

const std::filesystem::path& SettingsManager::GetFilePath() const noexcept
{
    return filePath;
}

bool SettingsManager::Load()
{
    std::ifstream file(filePath);
    if (!file)
    {
        settings = defaults;
        return Save();
    }

    try
    {
        const Json data{ Json::parse(file) };
        if (!data.is_object())
        {
            settings = defaults;
            return Save();
        }

        settings = Deserialize(data, defaults);
        return true;
    }
    catch (const Json::exception&)
    {
        settings = defaults;
        return Save();
    }
}

bool SettingsManager::Save() const
{
    std::error_code error;
    std::filesystem::create_directories(filePath.parent_path(), error);
    if (error)
        return false;

    std::filesystem::path temporaryPath{ filePath };
    temporaryPath += ".tmp";

    {
        std::ofstream file(temporaryPath, std::ios::trunc);
        if (!file)
            return false;

        file << Serialize(settings).dump(4) << '\n';
        if (!file)
            return false;
    }

    return ReplaceFile(temporaryPath, filePath);
}

bool SettingsManager::ResetToDefaults()
{
    settings = defaults;
    return Save();
}

GameSettings SettingsManager::CreateDefaults()
{
    GameSettings result;
    const sf::Vector2u desktopSize{ sf::VideoMode::getDesktopMode().size };
    if (desktopSize.x > 0u && desktopSize.y > 0u)
        result.graphics.resolution = desktopSize;

    return result;
}

std::filesystem::path SettingsManager::ResolveSettingsPath()
{
    char* localAppData{ nullptr };
    std::size_t length{ 0 };
    if (_dupenv_s(&localAppData, &length, "LOCALAPPDATA") == 0 && localAppData != nullptr)
    {
        const std::filesystem::path directory{
            std::filesystem::path(localAppData) / "Alone Bull Company" / "Until Last Asteroid"
        };
        std::free(localAppData);
        return directory / "settings.json";
    }

    std::free(localAppData);
    return std::filesystem::current_path() / "user_data" / "settings.json";
}
