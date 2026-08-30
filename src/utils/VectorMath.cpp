#include "VectorMath.h"

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
	const sf::Vector2f normalized{ Normalize(direction, fallback) };
	const float angle = std::atan2(normalized.y, normalized.x) + Random::Float(-spreadRadians, spreadRadians);
	return { std::cos(angle), std::sin(angle) };
}
