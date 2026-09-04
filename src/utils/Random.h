#pragma once

#include <random>

// Centralized source of randomness for gameplay systems (enemy spawns, drop
// chances, wave shuffling, visual jitter, etc.). Wraps a single shared
// std::mt19937_64 engine so callers don't need to seed or manage their own
// generators — just call Random::Int/Float wherever a random value is needed.
class Random
{
public:
	Random() = delete;

	[[nodiscard]] static int Int(int min, int max);
	[[nodiscard]] static float Float(float min, float max);

	// A coin flip: -1.f or 1.f with equal probability. For whenever "which
	// way" (spin clockwise or counterclockwise, mirror left or right, ...)
	// matters but "how much" doesn't -- keeps every such 50/50 choice
	// spelled the same way instead of each caller picking its own way to
	// draw one (Int(0,1)==0, Float(0,1)<0.5f, ...).
	[[nodiscard]] static float Sign();

private:
	static std::mt19937_64& GetGenerator();
};