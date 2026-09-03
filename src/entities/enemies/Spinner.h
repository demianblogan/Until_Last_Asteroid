#pragma once

#include "Enemy.h"

class Assets;
class World;

// Weaves back and forth in a sine-wave pattern while drifting across the
// screen, bouncing off the edges rather than leaving them, spinning
// cosmetically all the while (see the field comments below for the full
// movement breakdown). Periodically fires a volley from every weapon
// emitter at once, radiating outward rather than aiming at the player.
class Spinner final : public Enemy
{
public:
	Spinner(Assets& assets, World& world);

	void ConfigureApproachTarget(sf::Vector2f target) noexcept override;

	Type GetType() const noexcept override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	void ShootRadialVolley();
	[[nodiscard]] sf::Vector2f GetWeaponEmitterPosition(std::size_t index) const;

	// Spinner doesn't fly in a straight line -- it weaves along one while
	// drifting across the screen, like a sine wave traced over a straight
	// road. Every frame its velocity is built from two perpendicular parts
	// added together (see Update()):
	//   travelDirection * GetMovementSpeed()                    -- the "road"
	// + lateralDirection * (sin(movementPhase) * sineAmplitude) -- the "weave"
	// travelDirection is the road itself: a unit vector Spinner travels
	// along, reversed (per axis) whenever it nears a screen edge so it stays
	// on-screen, bouncing back and forth across the level over time.
	sf::Vector2f travelDirection;

	// The weave direction: always perpendicular to travelDirection (rotated
	// 90 degrees), recomputed every frame travelDirection changes. This is
	// what the sine wave actually oscillates along -- side to side relative
	// to the direction of travel, never forward/backward along it.
	sf::Vector2f lateralDirection;

	// How far the weave pushes sideways at its widest (sin peak), in units.
	// Larger = a wider, more exaggerated zig-zag across the travel line.
	float sineAmplitude = 0.f;

	// How fast the weave oscillates, in radians/second added to
	// movementPhase each frame -- higher = a tighter, more rapid wiggle.
	float sineFrequency = 0.f;

	// The sine wave's current position in its cycle (0..2*pi, wrapping
	// forever since it's never clamped). sin(movementPhase) is what actually
	// scales the lateral weave above; this is that angle, advancing at
	// sineFrequency radians/second.
	float movementPhase = 0.f;

	// Purely cosmetic sprite spin (degrees/second, see Update()'s
	// SetRotation call) -- entirely unrelated to the travel/weave movement
	// above; Spinner's facing has no bearing on where it's actually heading.
	float spinDirection = 1.f;

	float shootTimer = 0.f;
	float shootInterval = 0.f;
};
