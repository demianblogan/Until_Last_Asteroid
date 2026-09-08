#pragma once

#include <cstdint>
#include <cstddef>
#include <optional>

#include "core/Entity.h"
#include "gameplay/GameplayData.h"

class World;
class Assets;

namespace sf
{
	class Texture;
}

// Shared base for every projectile in the game (see the three concrete
// kinds below). Has no health of its own -- it deals its damage/knockback
// on contact and is destroyed, rather than surviving a hit -- and always
// travels in a straight line at a fixed speed (see SetDirection) unless a
// subclass steers it (homing) or ReflectToward flips it around entirely
// (a shield bouncing it back the way it came, becoming the opposite side's
// projectile in the process -- see IsReflected).
class Shot : public Entity
{
public:
	enum class VisualKind
	{
		Player,
		PlayerHoming,
		PlayerTriple,
		Helper,
		Enemy
	};

	Shot(Assets& assets, World& world, sf::Texture& texture, const GameplayData::ProjectileConfig& config,
		VisualKind visualKind, std::uint64_t playerAttackID = 0u);

	void Update(float deltaTime) override;

	[[nodiscard]] int GetDamage() const noexcept;
	[[nodiscard]] float GetKnockback() const noexcept;
	[[nodiscard]] std::uint64_t GetPlayerAttackID() const noexcept;

	void ReflectToward(const sf::Vector2f& targetPosition);

protected:
	// Points the shot along `direction`: its velocity becomes that heading at
	// the shot's fixed speed, and its sprite turns to face the same way. A
	// projectile has no steering independent of its velocity, so "where it's
	// aimed" and "how it moves" are one thing, set together here. `direction`
	// need not be unit length; a degenerate (zero) vector leaves the shot
	// pointing straight up.
	void SetHeading(const sf::Vector2f& direction) noexcept;

	// Rotates the current heading toward `targetPosition` by at most
	// `turnRateDegrees` per second (a gradual lock-on), then re-aims the shot
	// along it via SetHeading. No-op while the shot is momentarily stationary
	// or already sitting on the target. Shared by every homing shot kind.
	void SteerToward(const sf::Vector2f& targetPosition, float turnRateDegrees, float deltaTime) noexcept;

	[[nodiscard]] bool IsReflected() const noexcept;

private:
	float speed = 0.f;
	int damage = 0;
	float knockback = 0.f;
	VisualKind visualKind;
	std::uint64_t playerAttackID = 0u;
	bool isReflected = false;
};

// The player's regular shot -- fired straight from the ship's muzzle, or
// homing toward the nearest enemy (or boss part) when the homing-bullets
// bonus is active (see AcquireHomingTarget/UpdateHoming), gradually turning
// toward its target each frame rather than snapping onto it. Also used for
// the triple-shot bonus's two angled side shots (see VisualKind::PlayerTriple
// in Shot). Becomes an enemy projectile if reflected off a shield.
class PlayerShot final : public Shot
{
public:
	PlayerShot(Assets& assets, World& world, const sf::Vector2f& position, const sf::Vector2f& aimDirection,
		std::uint64_t attackID, bool needToPlaySound = true, bool isTripleShotVisual = false);

	void Update(float deltaTime) override;
	Type GetType() const noexcept override;
	bool IsCollidingWith(const Entity& other) const override;

private:
	void AcquireHomingTarget();
	void UpdateHoming(float deltaTime);

	const Entity* homingTarget = nullptr;
	std::optional<std::size_t> bossHomingTarget;
	bool isHomingEnabled = false;
};

// A generic enemy shot -- fired straight at wherever it was aimed at spawn
// time, never homing or correcting course afterward. Despite the name, this
// is used by every shooting enemy (Spinner, ReflectorGunship, ShooterSaucer),
// not just saucers specifically.
class SaucerShot final : public Shot
{
public:
	SaucerShot(Assets& assets, World& world, const sf::Vector2f& position, const sf::Vector2f& targetPosition,
		GameplayData::ProjectileKind projectileKind = GameplayData::ProjectileKind::Enemy,
		bool needToPlaySound = true);

	Type GetType() const noexcept override;
	bool IsCollidingWith(const Entity& other) const override;
};

// HelperBot's own shot -- homes toward the nearest enemy in front of the
// bot, similar to PlayerShot's homing-bullets behavior but always active
// (no bonus required) and with a simpler single-target search (no boss-part
// preference). Counts as an ally projectile, not the player's own.
class HelperShot final : public Shot
{
public:
	HelperShot(Assets& assets, World& world, const sf::Vector2f& position, const Entity* target);

	void Update(float deltaTime) override;
	[[nodiscard]] Type GetType() const noexcept override;
	[[nodiscard]] bool IsCollidingWith(const Entity& other) const override;

private:
	void AcquireTarget();
	void UpdateHoming(float deltaTime);

	const Entity* homingTarget = nullptr;
};