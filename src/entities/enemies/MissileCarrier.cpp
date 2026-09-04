#include "MissileCarrier.h"

#include <cmath>
#include <numbers>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "utils/VectorMath.h"

namespace
{
	// MissileCarrier's engine trail and launch direction should point downward
	// by default when velocity/aim is degenerate, matching its default facing.
	constexpr sf::Vector2f DegenerateDirectionFallback{ 0.f, -1.f };
}

MissileCarrier::MissileCarrier(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::MissileCarrier),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::MissileCarrier))
	, launchInterval(assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::MissileCarrier).actionInterval)
{}

Entity::Type MissileCarrier::GetType() const noexcept
{
	return Type::Enemy;
}

void MissileCarrier::Update(float deltaTime)
{
	const sf::Vector2f playerPosition{ GetWorld().GetPlayerPosition() };

	TurnTowards(playerPosition, GetRotationSpeed(), deltaTime);
	UpdatePatrolMovement(deltaTime);
	EmitEngineParticles(-VectorMath::Normalize(GetVelocity(), DegenerateDirectionFallback));

	launchTimer += deltaTime;

	if (launchTimer >= launchInterval)
	{
		launchTimer -= launchInterval;
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
	const float width = static_cast<float>(GetWorld().GetWidth());
	const float height = static_cast<float>(GetWorld().GetHeight());

	patrolTarget =
	{
		Random::Float(width * 0.2f, width * 0.8f),
		Random::Float(height * 0.18f, height * 0.82f)
	};
	hasPatrolTarget = true;
}

void MissileCarrier::UpdatePatrolMovement(float deltaTime)
{
	if (!hasPatrolTarget)
		ChooseCentralPatrolTarget();

	if (MoveToward(patrolTarget, GetMovementSpeed(), deltaTime))
		ChooseCentralPatrolTarget();
}

void MissileCarrier::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPosition(), GetVelocity(), 1.3f });
}

void MissileCarrier::LaunchMissile(const sf::Vector2f& target)
{
	const sf::Vector2f launcherPosition{ GetLauncherPosition() };
	const sf::Vector2f direction{ VectorMath::Normalize(target - launcherPosition, DegenerateDirectionFallback) };

	GetWorld().Effects().Add({ Rendering::EffectEventType::EnemyMuzzleFlash,launcherPosition, direction, 1.15f });
	GetWorld().Sound().AddSound(Config::Sound::EnemyShot, 0.72f);
	GetWorld().SpawnHomingMissile(launcherPosition, target);
}

sf::Vector2f MissileCarrier::GetLauncherPosition() const
{
	return GetWeaponEmitterPosition(0u);
}

void MissileCarrier::EmitEngineParticles(const sf::Vector2f& exhaustDirection)
{
	for (const GameplayData::NormalizedPoint& emitter : GetEngineEmitters())
	{
		GetWorld().Effects().Add(
			{ Rendering::EffectEventType::EnemyEngine,	TransformNormalizedPoint(emitter), exhaustDirection });
	}
}