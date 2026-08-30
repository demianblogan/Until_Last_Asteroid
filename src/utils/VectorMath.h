#pragma once

#include <SFML/System/Vector2.hpp>

// Small shared helpers for 2D vector math, used across core/entities/rendering
// so each caller doesn't reimplement the same degenerate-input guards.
class VectorMath
{
public:
	VectorMath() = delete;

	// Returns `vector` scaled to unit length, or `fallback` if `vector` is at
	// (or extremely close to) the origin, where a direction is undefined.
	//
	// SFML's own Vector2::normalized() doesn't cover this: it asserts on an
	// exact-zero vector (crashing instead of degrading), has no fallback
	// direction to hand back, and only guards the exact-zero case, not the
	// "extremely close to zero" one -- callers here routinely normalize
	// vectors built from subtraction or a stationary emitter's velocity,
	// where landing suspiciously close to (but not exactly) the origin is
	// the common case, not the rare one.
	[[nodiscard]] static sf::Vector2f Normalize(const sf::Vector2f& vector, sf::Vector2f fallback = { 1.f, 0.f });

	// A uniformly random unit vector (a random point on the unit circle).
	[[nodiscard]] static sf::Vector2f RandomDirection();

	// A random unit vector within `spreadRadians` of `direction`'s own
	// angle -- e.g. a cone of scatter around an impact's incoming direction.
	// `fallback` is used in `direction`'s place if it's degenerate, same as
	// Normalize().
	[[nodiscard]] static sf::Vector2f RandomDirectionAround(
		const sf::Vector2f& direction, float spreadRadians, sf::Vector2f fallback = { 1.f, 0.f });
};
