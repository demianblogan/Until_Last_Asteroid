#pragma once

#include "core/Entity.h"
#include "gameplay/GameplayData.h"
#include "gameplay/Health.h"

#include <string>

class Assets;
class World;

namespace sf
{
	class Texture;
}

// Shared base for every hostile ship/asteroid the player fights (see
// entities/enemies/ for the concrete list). Owns everything common to all of
// them: health and damage, score/pickup/part rewards on death (each
// independently toggleable, since a boss's health-bar sidekicks and
// campaign-specific spawns need different reward rules), the shared
// "flies to a point before starting its own pattern" approach-movement
// machinery, and default answers for the handful of yes/no questions World
// asks about any enemy (does it get knocked back, does it reflect
// projectiles, does it block the laser). Movement, shooting, and anything
// else about how a specific enemy actually behaves belongs entirely to the
// subclass -- Enemy itself only moves in a straight line (see Update()).
class Enemy : public Entity
{
public:
	Enemy(Assets& assets, World& world, sf::Texture& texture, const GameplayData::EnemyConfig& config);

	[[nodiscard]] int GetScoreValue() const noexcept;
	[[nodiscard]] int GetContactDamage() const noexcept;
	[[nodiscard]] float GetCollisionImpulse() const noexcept;
	[[nodiscard]] float GetSoundPitchMultiplier() const noexcept;
	[[nodiscard]] int GetCurrentHealth() const noexcept;

	void SetPickupDrop(const std::optional<GameplayData::SpawnGroup::PickupDropConfig>& drop);
	[[nodiscard]] std::optional<GameplayData::PickupKind> RollPickupDrop() const;
	void SetOrderedPickupDropCount(int count) noexcept;
	[[nodiscard]] int GetOrderedPickupDropCount() const noexcept;

	void SetPartDropID(std::string id);
	[[nodiscard]] const std::string& GetPartDropID() const noexcept;

	// Points this enemy at `target` and marks it as approaching -- used when
	// a wave's enemies enter from off-screen and need to fly to a central
	// point before falling into their own movement pattern. The default here
	// just records `target`/isApproachingCenter for UpdateApproach() below;
	// most overriding subclasses call this base version first and then set
	// whatever extra state their own pattern needs (e.g. Spinner recomputing
	// its travel direction toward the new target). Default no-op subclasses
	// (Meteor, KamikazeSaucer, LaserTurret, ShooterStation) simply never have
	// this called on them -- see the spawn.kind check in
	// GameplayState::SpawnConfiguredEnemy.
	virtual void ConfigureApproachTarget(sf::Vector2f target) noexcept;

	void SetRewardsEnabled(bool isEnabled) noexcept;
	[[nodiscard]] bool AreRewardsEnabled() const noexcept;
	void SetScoreRewardEnabled(bool isEnabled) noexcept;
	void SetPickupRewardsEnabled(bool isEnabled) noexcept;
	void SetPartRewardEnabled(bool isEnabled) noexcept;

	[[nodiscard]] bool IsScoreRewardEnabled() const noexcept;
	[[nodiscard]] bool ArePickupRewardsEnabled() const noexcept;
	[[nodiscard]] bool IsPartRewardEnabled() const noexcept;

	// Whether a hit (projectile or collision impact) should physically shove
	// this enemy backwards -- see the enemy.ApplyImpulse() call sites in
	// World. Default true (most enemies get pushed around by what they're
	// hit with); LaserTurret, ReflectorGunship and ShooterStation override
	// this to false so a hit can't shove them off their scripted path.
	[[nodiscard]] virtual bool AcceptsKnockback() const noexcept;

	// Whether this enemy currently reflects the player's projectiles back at
	// the player instead of taking damage from them (see
	// World::ResolvePlayerProjectileHit -- when true, the shot is redirected
	// toward the player and survives instead of dealing damage and being
	// destroyed). Default false; only ReflectorGunship overrides this,
	// tying it to whether its shield is currently up.
	[[nodiscard]] virtual bool IsProjectileReflectionActive() const noexcept;

	// Whether this enemy is immune to the player's laser beam. Despite the
	// name, this does NOT stop the beam from continuing on to hit whatever
	// else is behind this enemy -- World::DamageEnemiesWithPlayerLaser tests
	// every enemy in the beam's path independently and just skips damaging
	// the ones where this returns true. Default false; only ReflectorGunship
	// overrides it, tying it to whether its shield is currently up.
	[[nodiscard]] virtual bool BlocksPlayerLaser() const noexcept;

	[[nodiscard]] virtual bool CollidesWithPlayerProjectile(const Entity& projectile) const;
	[[nodiscard]] virtual sf::Vector2f GetPlayerProjectileImpactPosition(const Entity& projectile) const noexcept;
	[[nodiscard]] virtual bool TakeDamage(int damage);

	Type GetType() const noexcept override;

protected:
	// The ordinary "any ship in the sky" collision rule: hits the player, the
	// player's own shots (direct or ally), and enemy missiles; ignores
	// everything else (other enemies, asteroids, the player's laser -- which
	// goes through DamageEnemiesWithPlayerLaser instead). This covers most
	// enemies as-is; the ones with their own rule (a shield that needs
	// reflection routing, a destruction sequence that should stop colliding,
	// asteroids ignoring each other) override it instead of using this.
	[[nodiscard]] bool IsCollidingWith(const Entity& other) const override;

	void Update(float deltaTime) override;

	[[nodiscard]] float GetMovementSpeed() const noexcept;
	[[nodiscard]] float GetRotationSpeed() const noexcept;
	[[nodiscard]] const std::vector<GameplayData::NormalizedPoint>& GetWeaponEmitters() const noexcept;
	[[nodiscard]] const std::vector<GameplayData::NormalizedPoint>& GetEngineEmitters() const noexcept;

	// World position of weapon emitter `index` (authored 0..1-normalized in
	// GameplayData), transformed by this enemy's current position/rotation/
	// scale. Folds together the GetWeaponEmitters().at(index) +
	// TransformNormalizedPoint() pair every shooting subclass repeated.
	[[nodiscard]] sf::Vector2f GetWeaponEmitterPosition(std::size_t index) const;

	// Moves toward approachTarget (set via ConfigureApproachTarget) at
	// GetMovementSpeed(); once arrived, clears isApproachingCenter and
	// returns true so the caller can react to the arrival exactly once (e.g.
	// switch to its own patrol/pattern movement from here on). Returns false
	// every frame travel is still in progress. Subclasses that use the
	// shared approach state check `isApproachingCenter` themselves at the
	// top of Update() and call this instead of their own movement while it's
	// true -- see Spinner::Update for the reference shape.
	[[nodiscard]] bool UpdateApproach(float deltaTime) noexcept;

	sf::Vector2f approachTarget;
	bool isApproachingCenter = false;

	virtual void BeginDestruction();

private:
	Health health;
	int scoreValue = 0;
	int contactDamage = 0;
	float movementSpeed = 0.f;
	float collisionImpulse = 0.f;

	// Multiplier applied to the pitch of whichever sound plays for this
	// enemy -- both its hit sound and its destruction sound reuse this same
	// value, not just one specific sound.
	float soundPitchMultiplier = 1.f;

	float rotationSpeed = 0.f;

	std::vector<GameplayData::NormalizedPoint> weaponEmitters;
	std::vector<GameplayData::NormalizedPoint> engineEmitters;

	// Random pickup reward rolled on death via RollPickupDrop() -- a
	// weighted pool of pickup kinds plus the chance a drop happens at all.
	std::optional<GameplayData::SpawnGroup::PickupDropConfig> pickupDrop;

	// When set above 0, overrides pickupDrop entirely: instead of rolling
	// the random pool, this many pickups are pulled from the campaign's
	// scripted/ordered pickup queue (see World's use of GetOrderedPickupDropCount).
	// Used for campaign spawns with a specific guaranteed reward rather than
	// a random one.
	int orderedPickupDropCount = 0;

	// ID of the specific ship-upgrade Part this enemy drops on death, if any
	// and if the player hasn't already collected that part this campaign.
	// Empty means no part drop.
	std::string partDropID;

	bool isScoreRewardEnabled = true;
	bool arePickupRewardsEnabled = true;
	bool isPartRewardEnabled = true;
};
