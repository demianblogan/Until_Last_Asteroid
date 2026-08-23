#include "GamepadManager.h"

#include <algorithm>
#include <cmath>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>

namespace
{
	// USB vendor IDs, assigned by the USB Implementers Forum -- Microsoft's
	// and Sony's are fixed, publicly documented values, not something this
	// project chose. Used to tell an Xbox controller from a PlayStation one
	// (see RefreshConnection()) so the right button/axis layout is picked.
	constexpr unsigned int MicrosoftVendorID = 0x045Eu;
	constexpr unsigned int SonyVendorID = 0x054Cu;

	// Fraction (0-1) of a stick's travel that's ignored before input starts
	// registering, to absorb the small resting drift real analog sticks
	// have even when centered.
	constexpr float StickDeadZone = 0.18f;

	// Same threshold as StickDeadZone, just expressed on SFML's raw
	// -100..100 axis scale instead of the normalized 0-1 one: how far a
	// stick has to move before HandleEvent() below treats it as the player
	// actively using the gamepad, rather than as noise/drift from a stick
	// sitting still.
	constexpr float UsageDetectionThreshold = StickDeadZone * 100.f;

	// SFML reports joystick axes on a -100..100 scale. On an Xbox
	// controller, both analog triggers are reported on the single Z axis,
	// resting at 0 and moving toward -100 as the fire trigger is pulled
	// (this sign convention comes from the driver, not something chosen
	// here) -- this is how far past center it needs to move to count as
	// "fire" (see GetGameplayInput()).
	constexpr float TriggerThreshold = -20.f;

	sf::Vector2f ApplyDeadZone(sf::Vector2f value)
	{
		value /= 100.f;

		const float magnitude = std::sqrt(value.x * value.x + value.y * value.y);
		if (magnitude <= StickDeadZone)
			return { 0, 0 };

		// Rescales the part of the stick's travel beyond the dead zone back
		// out to the full 0..1 range, so movement starts smoothly at 0 right
		// past the dead zone's edge instead of jumping straight to
		// StickDeadZone's magnitude. clamp() is just a safety net for
		// sticks that report a hair past 1.0 at full deflection.
		const float adjustedMagnitude = std::clamp((magnitude - StickDeadZone) / (1.f - StickDeadZone), 0.f, 1.f);

		return value / magnitude * adjustedMagnitude;
	}
}

GamepadManager::GamepadManager()
{
	RefreshConnection();
}

void GamepadManager::HandleEvent(const sf::Event& event)
{
	if (event.is<sf::Event::JoystickConnected>() ||
		event.is<sf::Event::JoystickDisconnected>())
	{
		RefreshConnection();
	}

	if (event.is<sf::Event::MouseMoved>() ||
		event.is<sf::Event::MouseButtonPressed>() ||
		event.is<sf::Event::KeyPressed>())
	{
		isInUse = false;
	}
	else if (event.is<sf::Event::JoystickButtonPressed>())
	{
		isInUse = true;
	}
	else if (const auto* moved = event.getIf<sf::Event::JoystickMoved>())
	{
		if (IsActiveJoystick(moved->joystickId) && std::abs(moved->position) > UsageDetectionThreshold)
			isInUse = true;
	}
}

GamepadManager::GameplayInput GamepadManager::GetGameplayInput() const
{
	if (!activeJoystick)
		return {};

	GameplayInput input;
	input.movement = ReadStick(false);

	const sf::Vector2f aimDirection = ReadStick(true);
	const float aimLengthSquared = aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y;

	if (aimLengthSquared > 0.f)
		input.aimDirection = aimDirection / std::sqrt(aimLengthSquared);

	if (layout == Layout::PlayStation)
	{
		// R2, matching this layout's own button numbering (see
		// GetNavigationAction()/IsPausePressed(): Confirm=1, Back=2,
		// Pause=9 follow the same DualShock button order).
		constexpr unsigned int FireButton = 7u;

		input.isFiring = IsButtonPressed(FireButton);
	}
	else
	{
		const unsigned int id = *activeJoystick;
		if (sf::Joystick::hasAxis(id, sf::Joystick::Axis::Z))
		{
			input.isFiring = sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::Z) < TriggerThreshold;
		}
		else
		{
			// Fallback for the rare controller that doesn't report a Z axis
			// at all. Deliberately not button 7 -- for this layout, that's
			// pauseButton (see IsPausePressed()), and reusing it here would
			// make pressing Start also register as firing. Right bumper
			// (RB) is the least surprising substitute for a missing trigger.
			constexpr unsigned int FireButtonFallback = 5u;

			input.isFiring = IsButtonPressed(FireButtonFallback);
		}
	}

	return input;
}

bool GamepadManager::IsPausePressed(const sf::Event& event) const
{
	const auto* pressed = event.getIf<sf::Event::JoystickButtonPressed>();
	if (pressed == nullptr || !IsActiveJoystick(pressed->joystickId))
		return false;

	// Options on DualShock, Start on Xbox -- each layout's own "open the system menu" button.
	constexpr unsigned int PlayStationPauseButton = 9u;
	constexpr unsigned int XboxPauseButton = 7u;

	const unsigned int pauseButton = (layout == Layout::PlayStation) ? PlayStationPauseButton : XboxPauseButton;

	return pressed->button == pauseButton;
}

GamepadManager::NavigationAction GamepadManager::GetNavigationAction(const sf::Event& event) const
{
	if (const auto* moved = event.getIf<sf::Event::JoystickMoved>())
	{
		if (!IsActiveJoystick(moved->joystickId))
			return NavigationAction::None;

		constexpr float DpadThreshold = 50.f;
		constexpr float StickNavigationThreshold = 65.f;

		if (moved->axis == sf::Joystick::Axis::X)
		{
			if (moved->position < -StickNavigationThreshold)
				return NavigationAction::Left;
			else if (moved->position > StickNavigationThreshold)
				return NavigationAction::Right;
		}
		else if (moved->axis == sf::Joystick::Axis::Y)
		{
			if (moved->position < -StickNavigationThreshold)
				return NavigationAction::Up;
			else if (moved->position > StickNavigationThreshold)
				return NavigationAction::Down;
		}
		else if (moved->axis == sf::Joystick::Axis::PovX)
		{
			if (moved->position < -DpadThreshold)
				return NavigationAction::Left;
			else if (moved->position > DpadThreshold)
				return NavigationAction::Right;
		}
		else if (moved->axis == sf::Joystick::Axis::PovY)
		{
			if (moved->position > DpadThreshold)
				return NavigationAction::Up;
			else if (moved->position < -DpadThreshold)
				return NavigationAction::Down;
		}
		else
		{
			return NavigationAction::None;
		}
	}

	const auto* pressed = event.getIf<sf::Event::JoystickButtonPressed>();
	if (pressed == nullptr || !IsActiveJoystick(pressed->joystickId))
		return NavigationAction::None;

	// Circle/Square on DualShock, A/B on Xbox -- each layout's own
	// "confirm"/"back" buttons.
	constexpr unsigned int PlayStationConfirmButton = 1u;
	constexpr unsigned int XboxConfirmButton = 0u;
	constexpr unsigned int PlayStationBackButton = 2u;
	constexpr unsigned int XboxBackButton = 1u;

	const unsigned int confirmButton =
		(layout == Layout::PlayStation) ? PlayStationConfirmButton : XboxConfirmButton;
	const unsigned int backButton =
		(layout == Layout::PlayStation) ? PlayStationBackButton : XboxBackButton;

	if (pressed->button == confirmButton)
		return NavigationAction::Confirm;
	else if (pressed->button == backButton)
		return NavigationAction::Back;
	else
		return NavigationAction::None;
}

bool GamepadManager::IsConnected() const noexcept
{
	return activeJoystick.has_value();
}

bool GamepadManager::IsInUse() const noexcept
{
	return isInUse;
}

GamepadManager::Layout GamepadManager::GetLayout() const noexcept
{
	return layout;
}

void GamepadManager::RefreshConnection()
{
	activeJoystick.reset();
	layout = Layout::Generic;

	for (unsigned int id = 0u; id < sf::Joystick::Count; id++)
	{
		if (!sf::Joystick::isConnected(id))
			continue;

		activeJoystick = id;

		const sf::Joystick::Identification identification{ sf::Joystick::getIdentification(id) };

		if (identification.vendorId == MicrosoftVendorID)
			layout = Layout::Xbox;
		else if (identification.vendorId == SonyVendorID)
			layout = Layout::PlayStation;

		return;
	}
}

bool GamepadManager::IsActiveJoystick(unsigned int joystickID) const noexcept
{
	return activeJoystick.has_value() && *activeJoystick == joystickID;
}

bool GamepadManager::IsButtonPressed(unsigned int button) const
{
	return activeJoystick.has_value() &&
		button < sf::Joystick::getButtonCount(*activeJoystick) &&
		sf::Joystick::isButtonPressed(*activeJoystick, button);
}

sf::Vector2f GamepadManager::ReadStick(bool isRightStick) const
{
	if (!activeJoystick)
		return {};

	const unsigned int id = *activeJoystick;
	sf::Joystick::Axis horizontal = sf::Joystick::Axis::X;
	sf::Joystick::Axis vertical = sf::Joystick::Axis::Y;

	if (isRightStick)
	{
		// X/Y/Z/R/U/V are generic axis slots inherited from the old
		// DirectInput joystick API -- SFML doesn't know which physical
		// control on a given controller maps to which letter, only the
		// device's own driver does. Which slot the right stick actually
		// lands on differs by controller family:
		if (layout == Layout::PlayStation)
		{
			// DualShock/DualSense report the right stick on Z/R.
			horizontal = sf::Joystick::Axis::Z;
			vertical = sf::Joystick::Axis::R;
		}
		else
		{
			// Xbox controllers (via XInput) report the right stick on U/V
			// instead, because Z there is already taken by the combined
			// trigger axis (see TriggerThreshold in GetGameplayInput()).
			horizontal = sf::Joystick::Axis::U;
			vertical = sf::Joystick::Axis::V;
			if (!sf::Joystick::hasAxis(id, horizontal) || !sf::Joystick::hasAxis(id, vertical))
			{
				// U/V missing means this isn't a recognized Xbox
				// controller (Layout::Generic) -- fall back to Z/R as a
				// best-effort guess, since that's what PlayStation-style
				// controllers use for the same purpose.
				horizontal = sf::Joystick::Axis::Z;
				vertical = sf::Joystick::Axis::R;
			}
		}
	}

	if (!sf::Joystick::hasAxis(id, horizontal) || !sf::Joystick::hasAxis(id, vertical))
		return {};

	return ApplyDeadZone(
		{
			sf::Joystick::getAxisPosition(id, horizontal),
			sf::Joystick::getAxisPosition(id, vertical)
		}
	);
}