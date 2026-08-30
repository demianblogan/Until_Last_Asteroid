#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

enum class WindowMode
{
	Fullscreen,
	Windowed,
	Borderless
};

enum class Language
{
	English,
	Spanish,
	Russian,
	Ukrainian,
	Arabic
};

struct LocalizationSettings
{
	Language language = Language::English;

	// True once the player has explicitly picked a language (on the
	// LanguageSelect screen), as opposed to still running on the English
	// default. Used to decide whether CompanySplashState should show that
	// screen on startup or skip straight to the main menu.
	bool isLanguageChosen = false;
};

struct GraphicsSettings
{
	sf::Vector2u resolution = { 1920u, 1080u };
	WindowMode windowMode = WindowMode::Fullscreen;
	bool needToShowFPS = false;
	bool isVSyncEnabled = true;
	unsigned int frameRateLimit = 0u;
	bool arePostEffectsEnabled = true;
};

struct AudioSettings
{
	float musicVolume = 100.f;
	float soundVolume = 100.f;
};

enum class RebindableInputDevice
{
	Keyboard,
	Mouse
};

struct ControlBinding
{
	RebindableInputDevice device = RebindableInputDevice::Keyboard;
	int code = static_cast<int>(sf::Keyboard::Key::Unknown);
};

struct ControlSettings
{
	ControlBinding moveUp = { RebindableInputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::W) };
	ControlBinding moveDown = { RebindableInputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::S) };
	ControlBinding moveLeft = { RebindableInputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::A) };
	ControlBinding moveRight = { RebindableInputDevice::Keyboard, static_cast<int>(sf::Keyboard::Key::D) };
	ControlBinding fire = { RebindableInputDevice::Mouse, static_cast<int>(sf::Mouse::Button::Left) };
};

struct GameplaySettings
{
	bool isScreenShakeEnabled = true;
	bool needToShowScorePopups = true;
};

struct GamepadSettings
{
	bool isVibrationEnabled = true;
	bool isAdaptiveTriggersEnabled = true;
	bool isControllerLightbarEnabled = true;
};

struct GameSettings
{
	static constexpr int FormatVersion = 4;

	LocalizationSettings localization;
	GraphicsSettings graphics;
	AudioSettings audio;
	GameplaySettings gameplay;
	ControlSettings controls;
	GamepadSettings gamepad;
};