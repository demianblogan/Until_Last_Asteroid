#include "Enemy.h"

#include <cmath>
#include <numbers>
#include "assets/Assets.h"
#include "utils/Random.h"

Enemy::Enemy(Assets& assets, World& world, sf::Texture& texture,
	const GameplayData::EnemyConfig& config)
	: Entity(assets, world, texture, config.visualScale, config.collisionRadius,
		config.collisionCircles)
	, health(config.maximumHealth)
	, scoreValue(config.score)
	, contactDamage(config.contactDamage)
	, movementSpeed(config.speed)
	, collisionImpulse(config.collisionImpulse)
	, actionInterval(config.actionInterval)
	, fragmentSpeed(config.fragmentSpeed)
	, soundPitch(config.soundPitch)
	, rotationSpeed(config.rotationSpeed)
	, weaponEmitters(config.weaponEmitters)
	, engineEmitters(config.engineEmitters)
{
	constexpr float TwoPi{ 2.f * std::numbers::pi_v<float> };
	const float angle{ Random::Float(0.f, TwoPi) };
	const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
	SetVelocity(direction * movementSpeed);
}

int Enemy::GetScoreValue() const noexcept { return scoreValue; }
int Enemy::GetContactDamage() const noexcept { return contactDamage; }
float Enemy::GetCollisionImpulse() const noexcept { return collisionImpulse; }
float Enemy::GetSoundPitch() const noexcept { return soundPitch; }
int Enemy::GetCurrentHealth() const noexcept { return health.GetCurrent(); }

void Enemy::SetPickupDrop(
	const std::optional<GameplayData::SpawnGroup::PickupDropConfig>& drop)
{
	pickupDrop = drop;
}

std::optional<GameplayData::PickupKind> Enemy::RollPickupDrop() const
{
	if (!pickupDrop || pickupDrop->pool.empty() || pickupDrop->chance <= 0.f)
		return std::nullopt;
	if (pickupDrop->chance < 1.f &&
		Random::Float(0.f, 1.f) > pickupDrop->chance)
	{
		return std::nullopt;
	}

	float totalWeight{ 0.f };
	for (const auto& pickup : pickupDrop->pool)
		totalWeight += pickup.weight;
	float selection{ Random::Float(0.f, totalWeight) };
	for (const auto& pickup : pickupDrop->pool)
	{
		selection -= pickup.weight;
		if (selection <= 0.f)
			return pickup.kind;
	}
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

void Enemy::SetPartDropID(std::string id) { partDropID = std::move(id); }
const std::string& Enemy::GetPartDropID() const noexcept { return partDropID; }
void Enemy::SetRewardsEnabled(bool enabled) noexcept
{
	scoreRewardEnabled = enabled;
	pickupRewardsEnabled = enabled;
	partRewardEnabled = enabled;
}
bool Enemy::AreRewardsEnabled() const noexcept
{
	return scoreRewardEnabled || pickupRewardsEnabled || partRewardEnabled;
}
void Enemy::SetScoreRewardEnabled(bool enabled) noexcept { scoreRewardEnabled = enabled; }
void Enemy::SetPickupRewardsEnabled(bool enabled) noexcept { pickupRewardsEnabled = enabled; }
void Enemy::SetPartRewardEnabled(bool enabled) noexcept { partRewardEnabled = enabled; }
bool Enemy::IsScoreRewardEnabled() const noexcept { return scoreRewardEnabled; }
bool Enemy::ArePickupRewardsEnabled() const noexcept { return pickupRewardsEnabled; }
bool Enemy::IsPartRewardEnabled() const noexcept { return partRewardEnabled; }
bool Enemy::AcceptsKnockback() const noexcept { return true; }
bool Enemy::IsProjectileReflectionActive() const noexcept { return false; }
bool Enemy::BlocksPlayerLaser() const noexcept { return false; }
bool Enemy::CollidesWithPlayerProjectile(const Entity& projectile) const
{
	return CheckCollision(projectile);
}
sf::Vector2f Enemy::GetPlayerProjectileImpactPosition(
	const Entity& projectile) const noexcept
{
	return projectile.GetPosition();
}

bool Enemy::TakeDamage(int damage)
{
	const bool damageApplied{ health.ApplyDamage(damage) };
	if (!damageApplied)
		return false;
	if (health.IsDepleted())
	{
		BeginDestruction();
		return true;
	}

	FlashOnHit(GetAssets().GetGameplayData().GetHitFlashDuration());
	return false;
}

void Enemy::BeginDestruction() { Destroy(); }

Entity::Type Enemy::GetType() const noexcept { return Type::Enemy; }

void Enemy::Update(float deltaTime)
{
	Move(deltaTime);
}

float Enemy::GetMovementSpeed() const noexcept { return movementSpeed; }
float Enemy::GetActionInterval() const noexcept { return actionInterval; }
float Enemy::GetFragmentSpeed() const noexcept { return fragmentSpeed; }
float Enemy::GetRotationSpeed() const noexcept { return rotationSpeed; }
const std::vector<GameplayData::NormalizedPoint>& Enemy::GetWeaponEmitters() const noexcept
{
	return weaponEmitters;
}

const std::vector<GameplayData::NormalizedPoint>& Enemy::GetEngineEmitters() const noexcept
{
	return engineEmitters;
}
