#include "VectorMath.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "utils/Random.h"

namespace
{
	// Guards the division below against a meaningless direction when `vector`
	// is at (or extremely close to) the origin. Squared because we're
	// comparing against a squared length, not a length: this is (0.01f *
	// 0.01f), so vectors shorter than 0.01 units are treated as zero.
	constexpr float NormalizeEpsilonSquared = 0.0001f;
}

sf::Vector2f VectorMath::RotateToward(
	sf::Vector2f heading, sf::Vector2f desired, sf::Angle maximumStep, sf::Vector2f fallback)
{
	if (heading.lengthSquared() <= NormalizeEpsilonSquared)
		return Normalize(fallback);
	if (desired.lengthSquared() <= NormalizeEpsilonSquared)
		return heading.normalized();

	// angleTo() already gives the shortest signed turn (wrapped to
	// (-180, 180]), so clamping it to +/- maximumStep and rotating by the
	// result is the whole operation.
	const sf::Angle step{ std::clamp(heading.angleTo(desired), -maximumStep, maximumStep) };
	return heading.rotatedBy(step).normalized();
}

sf::Vector2f VectorMath::Normalize(const sf::Vector2f& vector, sf::Vector2f fallback)
{
	const float lengthSquared = vector.x * vector.x + vector.y * vector.y;
	if (lengthSquared <= NormalizeEpsilonSquared)
		return fallback;

	return vector / std::sqrt(lengthSquared);
}

sf::Vector2f VectorMath::RandomDirection()
{
	const float angle = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);
	return { std::cos(angle), std::sin(angle) };
}

sf::Vector2f VectorMath::RandomDirectionAround(const sf::Vector2f& direction, float spreadRadians, sf::Vector2f fallback)
{
	// angle() only cares about direction, not length, so there's no need to
	// pay for a full normalize here -- `direction` is used as-is unless it's
	// degenerate, in which case `fallback` stands in for it.
	const sf::Vector2f baseDirection{ direction.lengthSquared() > NormalizeEpsilonSquared ? direction : fallback };
	const sf::Angle angle{ baseDirection.angle() + sf::radians(Random::Float(-spreadRadians, spreadRadians)) };
	return sf::Vector2f{ 1.f, angle };
}
