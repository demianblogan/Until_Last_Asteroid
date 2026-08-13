#include "MissileCarrier.h"

#include <cmath>
#include <numbers>
#include "assets/AssetStore.h"
#include "core/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"

namespace
{
	sf::Vector2f Normalize(const sf::Vector2f& vector)
	{
		const float lengthSquared{ vector.x * vector.x + vector.y * vector.y };
		if (lengthSquared <= 0.0001f)
			return { 0.f, -1.f };
		return vector / std::sqrt(lengthSquared);
	}
}

MissileCarrier::MissileCarrier(AssetStore& assets, World& world)
	: Enemy(
		assets,
		world,
		assets.Textures().Get(Config::Texture::MissileCarrier),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::MissileCarrier))
{
}

Entity::Type MissileCarrier::GetType() const noexcept
{
	return Type::Enemy;
}

bool MissileCarrier::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Player ||
		other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

void MissileCarrier::Update(float deltaTime)
{
	const sf::Vector2f playerPosition{ GetWorld().GetPlayerPosition() };
	const sf::Vector2f toPlayer{ playerPosition - GetPosition() };
	const float angle{ std::atan2(toPlayer.y, toPlayer.x) };
	SetRotation(sf::radians(angle + std::numbers::pi_v<float> * 0.5f));
	UpdatePatrolMovement(deltaTime);
	EmitEngineParticles(-Normalize(GetVelocity()));

	launchTimer += deltaTime;
	if (launchTimer >= GetActionInterval())
	{
		launchTimer -= GetActionInterval();
		LaunchMissile(playerPosition);
	}
}

void MissileCarrier::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	patrolTarget = target;
	hasPatrolTarget = true;
}

void MissileCarrier::ChooseCentralPatrolTarget()
{
	const float width{ static_cast<float>(GetWorld().GetWidth()) };
	const float height{ static_cast<float>(GetWorld().GetHeight()) };
	patrolTarget = {
		Random::Float(width * 0.2f, width * 0.8f),
		Random::Float(height * 0.18f, height * 0.82f) };
	hasPatrolTarget = true;
}

void MissileCarrier::UpdatePatrolMovement(float deltaTime)
{
	if (!hasPatrolTarget)
		ChooseCentralPatrolTarget();
	const sf::Vector2f delta{ patrolTarget - GetPosition() };
	const float distance{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
	const float step{ GetMovementSpeed() * deltaTime };
	if (distance <= std::max(step, 0.001f))
	{
		SetPosition(patrolTarget);
		ChooseCentralPatrolTarget();
		return;
	}
	SetVelocity(delta / distance * GetMovementSpeed());
	Move(deltaTime);
}

void MissileCarrier::OnDestroy()
{
	GetWorld().AddSound(Config::Sound::ShipExplosion);
	GetWorld().AddEffectEvent({
		World::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), 1.3f });
}

void MissileCarrier::LaunchMissile(const sf::Vector2f& target)
{
	const sf::Vector2f launcherPosition{ GetLauncherPosition() };
	const sf::Vector2f direction{ Normalize(target - launcherPosition) };
	GetWorld().AddEffectEvent({
		World::EffectEventType::EnemyMuzzleFlash,
		launcherPosition, direction, 1.15f });
	GetWorld().AddSound(Config::Sound::EnemyShot, 0.72f);
	GetWorld().SpawnHomingMissile(launcherPosition, target);
}

sf::Vector2f MissileCarrier::GetLauncherPosition() const
{
	return GetEmitterPosition(GetWeaponEmitters().front());
}

void MissileCarrier::EmitEngineParticles(const sf::Vector2f& exhaustDirection)
{
	for (const GameplayData::NormalizedPoint& emitter : GetEngineEmitters())
	{
		GetWorld().AddEffectEvent({
			World::EffectEventType::EnemyEngine,
			GetEmitterPosition(emitter), exhaustDirection });
	}
}

sf::Vector2f MissileCarrier::GetEmitterPosition(
	const GameplayData::NormalizedPoint& emitter) const
{
	const sf::Sprite& sprite{ GetSprite() };
	const sf::IntRect textureRect{ sprite.getTextureRect() };
	const sf::Vector2f localPosition{
		static_cast<float>(textureRect.position.x) +
			static_cast<float>(textureRect.size.x) * emitter.x,
		static_cast<float>(textureRect.position.y) +
			static_cast<float>(textureRect.size.y) * emitter.y };
	return sprite.getTransform().transformPoint(localPosition);
}
