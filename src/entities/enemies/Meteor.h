#pragma once

#include "Enemy.h"

class Assets;
class World;

namespace sf
{
	class Texture;
}

// The simplest enemy: drifts in a straight line (picked once at spawn, see
// Enemy's constructor) with a slow cosmetic spin, never shoots, and never
// gets steered toward an approach target the way most other enemies do --
// it just keeps going in whatever direction it started in, for its whole
// life. A Big meteor splits into two Small ones when destroyed (see
// OnDestroy); a Small one just breaks apart for good.
class Meteor final : public Enemy
{
public:
	enum class Size
	{
		Small,
		Big
	};

	Meteor(Assets& assets, World& world, Size size);

	[[nodiscard]] Size GetSize() const noexcept;
	void SetFragmentSpawningEnabled(bool isEnabled) noexcept;
	Type GetType() const noexcept override;
	bool IsCollidingWith(const Entity& other) const override;
	void OnDestroy() override;

private:
	void Update(float deltaTime) override;

	static const GameplayData::EnemyConfig& GetConfig(Assets& assets, Size size);
	static sf::Texture& GetRandomTexture(Assets& assets, Size size);

	Size size;
	bool isFragmentSpawningEnabled = true;

	// Purely cosmetic spin, in degrees per second -- applied every frame in
	// Update() to the sprite's rotation. Unrelated to movement (GetVelocity)
	// or to GetRotationSpeed(); it's randomized once at spawn (a fraction of
	// GetRotationSpeed(), in a random left/right direction) purely so
	// tumbling asteroids don't all spin at an identical rate.
	float angularVelocity = 0.f;

	// Launch speed given to the smaller "fragment" meteors a Big meteor
	// splits into when destroyed (see OnDestroy) -- lives here rather than
	// on the shared Enemy base since only Meteor ever splits into fragments.
	float fragmentSpeed = 0.f;
};
