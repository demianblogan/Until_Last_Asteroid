#include "Enemy.h"

#include "assets/Assets.h"
#include "utils/Random.h"

Enemy::Enemy(Assets& assets, World& world, sf::Texture& texture, const GameplayData::EnemyConfig& config)
	: Entity(assets, world, texture, config.visualScale, config.collisionRadius, config.collisionCircles)
	, health(config.maximumHealth)
	, scoreValue(config.score)
	, contactDamage(config.contactDamage)
	, movementSpeed(config.speed)
	, collisionImpulse(config.collisionImpulse)
	, soundPitchMultiplier(config.soundPitch)
	, rotationSpeed(config.rotationSpeed)
	, weaponEmitters(config.weaponEmitters)
	, engineEmitters(config.engineEmitters)
{}

int Enemy::GetScoreValue() const noexcept
{
	return scoreValue;
}

int Enemy::GetContactDamage() const noexcept
{
	return contactDamage;
}

float Enemy::GetCollisionImpulse() const noexcept
{
	return collisionImpulse;
}

float Enemy::GetSoundPitchMultiplier() const noexcept
{
	return soundPitchMultiplier;
}

int Enemy::GetCurrentHealth() const noexcept
{
	return health.GetCurrent();
}

void Enemy::SetPickupDrop(const std::optional<GameplayData::SpawnGroup::PickupDropConfig>& drop)
{
	pickupDrop = drop;
}

std::optional<GameplayData::PickupKind> Enemy::RollPickupDrop() const
{
	if (!pickupDrop || pickupDrop->pool.empty() || pickupDrop->chance <= 0.f)
		return std::nullopt;

	// First roll: does a pickup drop happen at all? pickupDrop->chance is a
	// 0..1 probability (e.g. 0.3 = 30% of kills from this enemy drop
	// something). Random::Float(0.f, 1.f) draws a random number in that same
	// 0..1 range; if it lands above the configured chance, this particular
	// death doesn't drop anything. The `chance < 1.f` check just skips this
	// roll entirely when the chance is 100% -- there's no point rolling dice
	// whose outcome can never fail.
	if (pickupDrop->chance < 1.f && Random::Float(0.f, 1.f) > pickupDrop->chance)
		return std::nullopt;

	// Second roll: given that a drop is happening, which pickup kind? Each
	// entry in the pool has a weight; summing them gives the total "size" of
	// the pool, then a random point within that total lands on exactly one
	// entry, proportionally to its weight -- a kind with weight 2 is twice
	// as likely to be picked as one with weight 1.
	float totalWeight = 0.f;
	for (const auto& pickup : pickupDrop->pool)
		totalWeight += pickup.weight;

	float selection = Random::Float(0.f, totalWeight);

	for (const auto& pickup : pickupDrop->pool)
	{
		selection -= pickup.weight;
		if (selection <= 0.f)
			return pickup.kind;
	}

	// Floating-point rounding could in theory leave `selection` still
	// positive after subtracting every weight; falling back to the last
	// entry guarantees a kind is always returned once we've committed to
	// dropping something.
	return pickupDrop->pool.back().kind;
}

void Enemy::SetOrderedPickupDropCount(int count) noexcept
{
	orderedPickupDropCount = std::max(0, count);
}

int Enemy::GetOrderedPickupDropCount() const noexcept
{
	return orderedPickupDropCount;
}

void Enemy::SetPartDropID(std::string id)
{
	partDropID = std::move(id);
}

const std::string& Enemy::GetPartDropID() const noexcept
{
	return partDropID;
}

void Enemy::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	approachTarget = target;
	isApproachingCenter = true;
}

void Enemy::SetRewardsEnabled(bool isEnabled) noexcept
{
	isScoreRewardEnabled = isEnabled;
	arePickupRewardsEnabled = isEnabled;
	isPartRewardEnabled = isEnabled;
}

bool Enemy::AreRewardsEnabled() const noexcept
{
	return isScoreRewardEnabled || arePickupRewardsEnabled || isPartRewardEnabled;
}

void Enemy::SetScoreRewardEnabled(bool isEnabled) noexcept
{
	isScoreRewardEnabled = isEnabled;
}

void Enemy::SetPickupRewardsEnabled(bool isEnabled) noexcept
{
	arePickupRewardsEnabled = isEnabled;
}

void Enemy::SetPartRewardEnabled(bool isEnabled) noexcept
{
	isPartRewardEnabled = isEnabled;
}

bool Enemy::IsScoreRewardEnabled() const noexcept
{
	return isScoreRewardEnabled;
}

bool Enemy::ArePickupRewardsEnabled() const noexcept
{
	return arePickupRewardsEnabled;
}

bool Enemy::IsPartRewardEnabled() const noexcept
{
	return isPartRewardEnabled;
}

bool Enemy::AcceptsKnockback() const noexcept
{
	return true;
}

bool Enemy::IsProjectileReflectionActive() const noexcept
{
	return false;
}

bool Enemy::BlocksPlayerLaser() const noexcept
{
	return false;
}

bool Enemy::IsCollidingWith(const Entity& other) const
{
	return (other.GetType() == Type::Player ||
		other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::Projectile_Ally ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

bool Enemy::CollidesWithPlayerProjectile(const Entity& projectile) const
{
	return CheckCollision(projectile);
}

sf::Vector2f Enemy::GetPlayerProjectileImpactPosition(const Entity& projectile) const noexcept
{
	return projectile.GetPosition();
}

bool Enemy::TakeDamage(int damage)
{
	const bool isDamageApplied = health.ApplyDamage(damage);

	if (!isDamageApplied)
		return false;

	if (health.IsDepleted())
	{
		BeginDestruction();
		return true;
	}

	FlashOnHit(GetAssets().GetGameplayData().GetHitFlashDuration());
	return false;
}

void Enemy::BeginDestruction()
{
	Destroy();
}

Entity::Type Enemy::GetType() const noexcept
{
	return Type::Enemy;
}

void Enemy::Update(float deltaTime)
{
	Move(deltaTime);
}

float Enemy::GetMovementSpeed() const noexcept
{
	return movementSpeed;
}

float Enemy::GetRotationSpeed() const noexcept
{
	return rotationSpeed;
}

const std::vector<GameplayData::NormalizedPoint>& Enemy::GetWeaponEmitters() const noexcept
{
	return weaponEmitters;
}

const std::vector<GameplayData::NormalizedPoint>& Enemy::GetEngineEmitters() const noexcept
{
	return engineEmitters;
}

sf::Vector2f Enemy::GetWeaponEmitterPosition(std::size_t index) const
{
	return TransformNormalizedPoint(weaponEmitters.at(index));
}

bool Enemy::UpdateApproach(float deltaTime) noexcept
{
	if (!MoveToward(approachTarget, GetMovementSpeed(), deltaTime))
		return false;

	isApproachingCenter = false;
	return true;
}
