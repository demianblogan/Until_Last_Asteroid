#include "Pulse.h"

#include <cmath>

float Pulse::Value(float clock, float frequency, float minimum, float range) noexcept
{
	return minimum + range * std::abs(std::sin(clock * frequency));
}