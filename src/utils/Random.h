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

private:
	static std::mt19937_64& GetGenerator();
};