#include "SettingsManager.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <string>
#include <system_error>

#include <nlohmann/json.hpp>
#include <SFML/Window/VideoMode.hpp>

#include "utils/AppDataPath.h"
#include "utils/SafeFileWrite.h"

namespace
{
	using Json = nlohmann::json;
	using SafeFileWrite::HasFailed;

	// Same reasoning as JSONIndentWidth in CampaignSaveManager.cpp: this is
	// the pretty-print indent width for the saved JSON, not an arbitrary
	// number.
	constexpr int JSONIndentWidth = 4;

	constexpr std::array<unsigned int, 7> SupportedFrameRateLimits =
	{
		0u, 30u, 60u, 120u, 144u, 240u, 360u
	};

	// Sanity bounds for a resolution loaded from settings.json: reject
	// anything a hand-edited or corrupted file might contain that couldn't
	// possibly be a real display resolution, so a garbage value can't be
	// forwarded straight into window/video mode creation.
	constexpr unsigned int MinSupportedWidth = 640u;
	constexpr unsigned int MinSupportedHeight = 480u;
	constexpr unsigned int MaxSupportedWidth = 16384u;
	constexpr unsigned int MaxSupportedHeight = 16384u;

	template <typename Value>
	Value ReadValue(const Json& object, const char* key, Value defaultValue)
	{
		const auto value = object.find(key);
		if (value == object.end())
			return defaultValue;

		try
		{
			return value->get<Value>();
		}
		catch (const Json::exception&)
		{
			return defaultValue;
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

		default:
			// Every real WindowMode value is handled explicitly above --
			// this is only reachable via a bad enum value (e.g. a future
			// enumerator added without updating this switch). Asserts loudly
			// in Debug instead of silently writing "fullscreen" to the save
			// file for a mode that was never actually Fullscreen.
			assert(false && "Unhandled WindowMode in ToString");
			return "fullscreen";
		}
	}

	std::string ToString(Language language)
	{
		switch (language)
		{
		case Language::English:
			return "en";
		case Language::Spanish:
			return "es";
		case Language::Russian:
			return "ru";
		case Language::Ukrainian:
			return "uk";
		case Language::Arabic:
			return "ar";

		default:
			// Every real Language value is handled explicitly above -- this
			// is only reachable via a bad enum value (e.g. a future
			// enumerator added without updating this switch). Asserts loudly
			// in Debug instead of silently writing "en" to the save file for
			// a language that was never actually English.
			assert(false && "Unhandled Language in ToString");
			return "en";
		}
	}

	WindowMode ParseWindowMode(const std::string& value, WindowMode fallback)
	{
		if (value == "fullscreen")
			return WindowMode::Fullscreen;
		else if (value == "windowed")
			return WindowMode::Windowed;
		else if (value == "borderless")
			return WindowMode::Borderless;
		else
			return fallback;
	}

	Language ParseLanguage(const std::string& value, Language fallback)
	{
		if (value == "en")
			return Language::English;
		else if (value == "es")
			return Language::Spanish;
		else if (value == "ru")
			return Language::Russian;
		else if (value == "uk")
			return Language::Ukrainian;
		else if (value == "ar")
			return Language::Arabic;
		else
			return fallback;
	}

	std::string ToString(RebindableInputDevice device)
	{
		return device == RebindableInputDevice::Mouse ? "mouse" : "keyboard";
	}

	RebindableInputDevice ParseInputDevice(const std::string& value, RebindableInputDevice fallback)
	{
		if (value == "keyboard")
			return RebindableInputDevice::Keyboard;
		else if (value == "mouse")
			return RebindableInputDevice::Mouse;
		else
			return fallback;
	}

	Json SerializeBinding(const ControlBinding& binding)
	{
		return
		{
			{ "device", ToString(binding.device) },
			{ "code", binding.code }
		};
	}

	ControlBinding DeserializeBinding(const Json& object, const ControlBinding& fallback)
	{
		if (!object.is_object())
			return fallback;

		ControlBinding result(fallback);
		result.device = ParseInputDevice(ReadValue(object, "device", ToString(fallback.device)), fallback.device);
		result.code = ReadValue(object, "code", fallback.code);

		const bool isValidKeyboard =
			result.device == RebindableInputDevice::Keyboard &&
			result.code >= 0 &&
			result.code < static_cast<int>(sf::Keyboard::KeyCount);

		const bool isValidMouse =
			result.device == RebindableInputDevice::Mouse &&
			result.code >= 0 &&
			result.code < static_cast<int>(sf::Mouse::ButtonCount);

		return isValidKeyboard || isValidMouse ? result : fallback;
	}

	Json Serialize(const GameSettings& settings)
	{
		return
		{
			{ "version", GameSettings::FormatVersion },
			{
				"localization",
				{
					{ "language", ToString(settings.localization.language) },
					{ "isLanguageChosen", settings.localization.isLanguageChosen }
				}
			},
			{
				"graphics",
				{
					{ "width", settings.graphics.resolution.x },
					{ "height", settings.graphics.resolution.y },
					{ "windowMode", ToString(settings.graphics.windowMode) },
					{ "needToShowFPS", settings.graphics.needToShowFPS },
					{ "isVSyncEnabled", settings.graphics.isVSyncEnabled },
					{ "frameRateLimit", settings.graphics.frameRateLimit },
					{ "arePostEffectsEnabled", settings.graphics.arePostEffectsEnabled }
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
				"gameplay",
				{
					{ "isScreenShakeEnabled", settings.gameplay.isScreenShakeEnabled },
					{ "needToShowScorePopups", settings.gameplay.needToShowScorePopups }
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
		GameSettings result(defaults);

		if (const auto localization(data.find("localization"));
			localization != data.end() && localization->is_object())
		{
			result.localization.language = ParseLanguage(
				ReadValue(*localization, "language", ToString(result.localization.language)),
				result.localization.language);

			result.localization.isLanguageChosen = ReadValue(
				*localization, "isLanguageChosen", result.localization.isLanguageChosen);
		}

		if (const auto graphics(data.find("graphics"));
			graphics != data.end() && graphics->is_object())
		{
			const unsigned int width = ReadValue(*graphics, "width", result.graphics.resolution.x);
			const unsigned int height = ReadValue(*graphics, "height", result.graphics.resolution.y);
			if (width >= MinSupportedWidth && height >= MinSupportedHeight &&
				width <= MaxSupportedWidth && height <= MaxSupportedHeight)
				result.graphics.resolution = { width, height };

			result.graphics.windowMode = ParseWindowMode(
				ReadValue(*graphics, "windowMode", ToString(result.graphics.windowMode)),
				result.graphics.windowMode);
			result.graphics.needToShowFPS = ReadValue(*graphics, "needToShowFPS", result.graphics.needToShowFPS);
			result.graphics.isVSyncEnabled = ReadValue(
				*graphics,
				"isVSyncEnabled",
				result.graphics.isVSyncEnabled);
			result.graphics.arePostEffectsEnabled = ReadValue(
				*graphics,
				"arePostEffectsEnabled",
				result.graphics.arePostEffectsEnabled);

			const unsigned int frameRateLimit =
				ReadValue(*graphics, "frameRateLimit", result.graphics.frameRateLimit);

			if (std::ranges::find(SupportedFrameRateLimits, frameRateLimit) != SupportedFrameRateLimits.end())
				result.graphics.frameRateLimit = frameRateLimit;
		}

		if (const auto gameplay(data.find("gameplay"));
			gameplay != data.end() && gameplay->is_object())
		{
			result.gameplay.isScreenShakeEnabled = ReadValue(
				*gameplay,
				"isScreenShakeEnabled",
				result.gameplay.isScreenShakeEnabled);
			result.gameplay.needToShowScorePopups = ReadValue(
				*gameplay,
				"needToShowScorePopups",
				result.gameplay.needToShowScorePopups);
		}

		if (const auto audio(data.find("audio"));
			audio != data.end() && audio->is_object())
		{
			const float legacyMusic = ReadValue(*audio, "music", result.audio.musicVolume);
			const float legacySounds = ReadValue(*audio, "sounds", result.audio.soundVolume);

			result.audio.musicVolume = std::clamp(ReadValue(*audio, "musicMaster", legacyMusic), 0.f, 100.f);
			result.audio.soundVolume = std::clamp(ReadValue(*audio, "soundsMaster", legacySounds), 0.f, 100.f);
		}

		if (const auto controls(data.find("controls"));
			controls != data.end() && controls->is_object())
		{
			const auto readBinding = [&controls](const char* key, const ControlBinding& fallback)
				{
					const auto binding = controls->find(key);
					return binding == controls->end() ? fallback : DeserializeBinding(*binding, fallback);
				};

			result.controls.moveUp = readBinding("moveUp", result.controls.moveUp);
			result.controls.moveDown = readBinding("moveDown", result.controls.moveDown);
			result.controls.moveLeft = readBinding("moveLeft", result.controls.moveLeft);
			result.controls.moveRight = readBinding("moveRight", result.controls.moveRight);
			result.controls.fire = readBinding("fire", result.controls.fire);
		}

		return result;
	}
}

SettingsManager::SettingsManager()
	: defaults(CreateDefaults())
	, settings(defaults)
	, settingsFilePath(ResolveSettingsPath())
{
	LoadSettings();
}

const GameSettings& SettingsManager::GetSettings() const noexcept
{
	return settings;
}

const GameSettings& SettingsManager::GetDefaults() const noexcept
{
	return defaults;
}

GameSettings& SettingsManager::EditSettings() noexcept
{
	return settings;
}

bool SettingsManager::LoadSettings()
{
	std::ifstream file(settingsFilePath);
	if (!file.is_open())
	{
		settings = defaults;
		return SaveSettings();
	}

	try
	{
		const Json data(Json::parse(file));
		if (!data.is_object())
		{
			settings = defaults;
			return SaveSettings();
		}

		settings = Deserialize(data, defaults);
		return true;
	}
	catch (const Json::exception&)
	{
		settings = defaults;
		return SaveSettings();
	}
}

bool SettingsManager::SaveSettings() const
{
	std::error_code error;
	std::filesystem::create_directories(settingsFilePath.parent_path(), error);
	if (HasFailed(error))
		return false;

	std::filesystem::path temporaryPath{ settingsFilePath };
	temporaryPath += ".tmp";

	{
		std::ofstream file(temporaryPath, std::ios::trunc);
		if (!file.is_open())
			return false;

		file << Serialize(settings).dump(JSONIndentWidth) << '\n';
		if (!file)
			return false;
	}

	return SafeFileWrite::ReplaceFileAtomically(temporaryPath, settingsFilePath);
}

bool SettingsManager::ResetSettingsToDefaults()
{
	settings = defaults;
	return SaveSettings();
}

GameSettings SettingsManager::CreateDefaults()
{
	GameSettings result;
	const sf::Vector2u desktopSize = { sf::VideoMode::getDesktopMode().size };
	if (desktopSize.x > 0u && desktopSize.y > 0u)
		result.graphics.resolution = desktopSize;

	return result;
}

std::filesystem::path SettingsManager::ResolveSettingsPath()
{
	return AppDataPath::Resolve("settings.json");
}