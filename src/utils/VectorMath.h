#pragma once

#include <SFML/System/Angle.hpp>
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

	// Turns `heading` toward `desired` by at most `maximumStep`, returning the
	// result as a unit vector -- the standard "gradual lock-on" step for a
	// homing projectile: each frame it rotates a bounded amount toward its
	// target instead of snapping straight at it. If `heading` is degenerate
	// the (normalized) `fallback` is returned; if only `desired` is
	// degenerate `heading` is kept (just normalized), i.e. no turn this frame.
	// Replaces the hand-rolled atan2()/atan2(sin,cos)/clamp/cos-sin dance
	// that was copy-pasted into every homing entity.
	[[nodiscard]] static sf::Vector2f RotateToward(
		sf::Vector2f heading, sf::Vector2f desired, sf::Angle maximumStep,
		sf::Vector2f fallback = { 1.f, 0.f });
};
