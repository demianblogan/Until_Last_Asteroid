#include "Spinner.h"

#include <cmath>
#include <numbers>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "utils/VectorMath.h"

namespace
{
	// Spinner's travel/emitter direction should point downward by default when
	// the source vector is degenerate, matching its default facing.
	constexpr sf::Vector2f DegenerateDirectionFallback{ 0.f, -1.f };
}

Spinner::Spinner(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::SpinnerPlatform),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Spinner))
{
	const auto& config = assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Spinner);

	travelDirection = VectorMath::RandomDirection();
	lateralDirection = travelDirection.perpendicular();
	sineAmplitude = config.sineAmplitude;
	sineFrequency = config.sineFrequency;
	movementPhase = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);
	spinDirection = Random::Sign();
	shootTimer = 0.f;
	shootInterval = config.actionInterval;
}

void Spinner::Update(float deltaTime)
{
	if (isApproachingCenter)
	{
		UpdateApproach(deltaTime);
	}
	else
	{
		const sf::Vector2f position{ GetPosition() };
		const float width = static_cast<float>(GetWorld().GetWidth());
		const float height = static_cast<float>(GetWorld().GetHeight());

		if ((position.x < width * 0.16f && travelDirection.x < 0.f) ||
			(position.x > width * 0.84f && travelDirection.x > 0.f))
			travelDirection.x = -travelDirection.x;

		if ((position.y < height * 0.16f && travelDirection.y < 0.f) ||
			(position.y > height * 0.84f && travelDirection.y > 0.f))
			travelDirection.y = -travelDirection.y;

		travelDirection = VectorMath::Normalize(travelDirection, DegenerateDirectionFallback);
		lateralDirection = travelDirection.perpendicular();
		movementPhase += 2.f * std::numbers::pi_v<float> *sineFrequency * deltaTime;

		SetVelocity(
			travelDirection * GetMovementSpeed() + lateralDirection * (std::sin(movementPhase) * sineAmplitude));

		Move(deltaTime);
	}

	SetRotation(GetRotation() + sf::degrees(GetRotationSpeed() * spinDirection * deltaTime));

	shootTimer += deltaTime;

	while (shootTimer >= shootInterval)
	{
		shootTimer -= shootInterval;
		ShootRadialVolley();
	}
}

void Spinner::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	Enemy::ConfigureApproachTarget(target);

	travelDirection = VectorMath::Normalize(target - GetPosition(), DegenerateDirectionFallback);
	lateralDirection = travelDirection.perpendicular();
}

void Spinner::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion,GetPosition(), GetVelocity(), 1.25f });
}

void Spinner::ShootRadialVolley()
{
	for (std::size_t i = 0; i < GetWeaponEmitters().size(); i++)
	{
		const sf::Vector2f emitter{ GetWeaponEmitterPosition(i) };
		const sf::Vector2f direction{ VectorMath::Normalize(emitter - GetPosition(), DegenerateDirectionFallback) };

		GetWorld().SpawnSaucerShot(emitter, emitter + direction * 100.f, GameplayData::ProjectileKind::Spinner, i == 0);
	}
}