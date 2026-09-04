#include "ShooterSaucer.h"

#include <algorithm>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"

ShooterSaucer::ShooterSaucer(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::SmallEnemySaucer),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Shooter))
	, shootInterval(assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Shooter).actionInterval)
{}

void ShooterSaucer::BeginMaterialization(float duration, const Entity* anchor) noexcept
{
	materializationDuration = std::max(0.05f, duration);
	materializationRemaining = materializationDuration;
	materializationAnchor = anchor;

	SetVelocity({});
	SetPresentation(0.18f, 0.f, sf::Color(255, 80, 80));
}

void ShooterSaucer::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	Enemy::ConfigureApproachTarget(target);
	hasPatrolTarget = false;
}

void ShooterSaucer::Update(float deltaTime)
{
	if (materializationRemaining > 0.f)
	{
		if (materializationAnchor != nullptr)
		{
			if (GetWorld().IsEntityActive(materializationAnchor))
				SetPosition(materializationAnchor->GetPosition());
			else
				materializationAnchor = nullptr;
		}

		materializationRemaining = std::max(0.f, materializationRemaining - deltaTime);
		const float progress = 1.f - materializationRemaining / materializationDuration;

		SetPresentation(0.18f + 0.82f * progress, progress, sf::Color(255, 80, 80));

		return;
	}

	const sf::Vector2f playerPosition{ GetWorld().GetPlayerPosition() };
	TurnTowards(playerPosition, GetRotationSpeed(), deltaTime);

	if (isApproachingCenter)
	{
		// hasPatrolTarget is already false from ConfigureApproachTarget, so
		// the patrol branch below picks a fresh target on its own the moment
		// isApproachingCenter flips false, without any extra handling here.
		UpdateApproach(deltaTime);
	}
	else
	{
		if (!hasPatrolTarget)
			ChooseCentralPatrolTarget();

		if (MoveToward(patrolTarget, GetMovementSpeed(), deltaTime))
			ChooseCentralPatrolTarget();
	}

	shootTimer += deltaTime;

	if (shootTimer >= shootInterval)
		Shoot();
}

void ShooterSaucer::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPosition(), GetVelocity(), 1.1f });
}

void ShooterSaucer::ChooseCentralPatrolTarget()
{
	const float width = static_cast<float>(GetWorld().GetWidth());
	const float height = static_cast<float>(GetWorld().GetHeight());

	patrolTarget =
	{
		Random::Float(width * 0.22f, width * 0.78f),
		Random::Float(height * 0.2f, height * 0.8f)
	};
	hasPatrolTarget = true;
}

void ShooterSaucer::Shoot()
{
	shootTimer -= shootInterval;

	const sf::Vector2f muzzle{ GetWeaponEmitterPosition(nextWeaponEmitter) };
	GetWorld().SpawnSaucerShot(muzzle, muzzle + GetForwardDirection() * 1000.f);
	nextWeaponEmitter = (nextWeaponEmitter + 1) % GetWeaponEmitters().size();
}