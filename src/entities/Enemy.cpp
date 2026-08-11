#include "Enemy.h"

#include <cmath>
#include <numbers>
#include "assets/AssetStore.h"
#include "utils/Random.h"

Enemy::Enemy(AssetStore& assets, World& world, sf::Texture& texture,
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

bool Enemy::TakeDamage(int damage)
{
	const bool damageApplied{ health.ApplyDamage(damage) };
	if (!damageApplied)
		return false;
	if (health.IsDepleted())
	{
		Destroy();
		return true;
	}

	FlashOnHit(GetAssets().GetGameplayData().GetHitFlashDuration());
	return false;
}

Entity::Type Enemy::GetType() const noexcept { return Type::Enemy; }

void Enemy::Update(float deltaTime)
{
	Move(deltaTime);
}

float Enemy::GetMovementSpeed() const noexcept { return movementSpeed; }
float Enemy::GetActionInterval() const noexcept { return actionInterval; }
float Enemy::GetFragmentSpeed() const noexcept { return fragmentSpeed; }
float Enemy::GetRotationSpeed() const noexcept { return rotationSpeed; }
const std::array<GameplayData::NormalizedPoint, 2>& Enemy::GetWeaponEmitters() const noexcept
{
	return weaponEmitters;
}
