#pragma once

// A small oscillating-brightness helper used throughout the game's visual
// effects -- shield glows, beam brightness, low-health/low-time flashes, UI
// highlights -- anywhere something should breathe/flicker smoothly over
// time rather than sit at a flat value.
class Pulse
{
public:
	Pulse() = delete;

	// Oscillates between `minimum` and `minimum + range`, cycling twice per
	// full sine period rather than once: sin() swings between -1 and 1, but
	// a "negative brightness" is meaningless, so the negative half is folded
	// back on top of the positive half (see the .cpp) -- the result bounces
	// between 0 and 1 twice as often as sin() itself would, then gets
	// rescaled into minimum..minimum+range so it never dims all the way to
	// zero (unless minimum is 0). `clock` is typically a running
	// elapsed-seconds counter (or a value that already IS the angle, with
	// `frequency` left at 1); `frequency` is in radians/second.
	[[nodiscard]] static float Value(float clock, float frequency, float minimum = 0.f, float range = 1.f) noexcept;
};
