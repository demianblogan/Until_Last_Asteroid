#pragma once

#include "input/gamepad/GamepadHaptics.h"

// Named vibration "feel" presets for this game's specific events (menu
// navigation, taking a hit, dying...). Kept separate from Haptics::
// GamepadHaptics itself so that class stays a generic, portable vibration
// primitive with no opinion about what any particular game should feel
// like -- these presets are this project's opinion, not the library's.
namespace VibrationProfiles
{
	struct Profile
	{
		float lowFrequencyMotor;
		float highFrequencyMotor;
		float durationSeconds;
	};

	// Light, quick tap -- main menu title/labels typing in, and getting hit
	// by an enemy projectile/laser/missile.
	inline constexpr Profile Light{ 0.15f, 0.35f, 0.08f };

	// Barely-there tick for moving the selection between menu items --
	// fires far more often than any other profile (every d-pad/stick move),
	// so it needs to be noticeably softer than Light or it reads as buzzy.
	inline constexpr Profile MenuNavigation{ 0.05f, 0.12f, 0.05f };

	// Firmer and longer than Light -- the player's ship physically colliding
	// with an enemy, asteroid, or boss hazard.
	inline constexpr Profile Collision{ 0.45f, 0.3f, 0.35f };

	// The strongest tier: the player's death.
	inline constexpr Profile Death{ 0.95f, 0.95f, 0.5f };

	// Re-armed every frame a boss destruction phase (ring/diamond/core) is
	// active, so it reads as one continuous strong vibration for as long as
	// that phase runs, fading out shortly after it ends.
	inline constexpr Profile BossExplosionSustain{ 0.85f, 0.65f, 0.15f };

	inline void Apply(Haptics::GamepadHaptics& haptics, const Profile& profile)
	{
		haptics.PulseVibration(profile.lowFrequencyMotor, profile.highFrequencyMotor, profile.durationSeconds);
	}
}
