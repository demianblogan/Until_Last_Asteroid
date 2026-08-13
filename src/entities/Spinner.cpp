#include "Spinner.h"

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

Spinner::Spinner(AssetStore& assets, World& world)
	: Enemy(
		assets,
		world,
		assets.Textures().Get(Config::Texture::SpinnerPlatform),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Spinner))
{
	const auto& config{
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Spinner) };
	const float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
	travelDirection = { std::cos(angle), std::sin(angle) };
	lateralDirection = { -travelDirection.y, travelDirection.x };
	sineAmplitude = config.sineAmplitude;
	sineFrequency = config.sineFrequency;
	movementPhase = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);
	spinDirection = Random::Float(0.f, 1.f) < 0.5f ? -1.f : 1.f;
	shootTimer = 0.f;
}

Entity::Type Spinner::GetType() const noexcept
{
	return Type::Enemy;
}

bool Spinner::IsCollideWith(const Entity& other) const
{
	return (other.GetType() == Type::Player ||
		other.GetType() == Type::Projectile_Player ||
		other.GetType() == Type::EnemyMissile) && CheckCollision(other);
}

void Spinner::Update(float deltaTime)
{
	if (approachingCenter)
	{
		const sf::Vector2f delta{ approachTarget - GetPosition() };
		const float distance{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
		const float step{ GetMovementSpeed() * deltaTime };
		if (distance <= std::max(step, 0.001f))
		{
			SetPosition(approachTarget);
			approachingCenter = false;
		}
		else
		{
			SetVelocity(delta / distance * GetMovementSpeed());
			Move(deltaTime);
		}
	}
	else
	{
		const sf::Vector2f position{ GetPosition() };
		const float width{ static_cast<float>(GetWorld().GetWidth()) };
		const float height{ static_cast<float>(GetWorld().GetHeight()) };
		if ((position.x < width * 0.16f && travelDirection.x < 0.f) ||
			(position.x > width * 0.84f && travelDirection.x > 0.f))
			travelDirection.x = -travelDirection.x;
		if ((position.y < height * 0.16f && travelDirection.y < 0.f) ||
			(position.y > height * 0.84f && travelDirection.y > 0.f))
			travelDirection.y = -travelDirection.y;
		travelDirection = Normalize(travelDirection);
		lateralDirection = { -travelDirection.y, travelDirection.x };
		movementPhase += 2.f * std::numbers::pi_v<float> * sineFrequency * deltaTime;
		SetVelocity(
			travelDirection * GetMovementSpeed() +
			lateralDirection * (std::sin(movementPhase) * sineAmplitude));
		Move(deltaTime);
	}

	SetRotation(GetRotation() + sf::degrees(
		GetRotationSpeed() * spinDirection * deltaTime));

	shootTimer += deltaTime;
	while (shootTimer >= GetActionInterval())
	{
		shootTimer -= GetActionInterval();
		ShootRadialVolley();
	}
}

void Spinner::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	approachTarget = target;
	travelDirection = Normalize(target - GetPosition());
	lateralDirection = { -travelDirection.y, travelDirection.x };
	approachingCenter = true;
}

void Spinner::OnDestroy()
{
	GetWorld().AddSound(Config::Sound::ShipExplosion);
	GetWorld().AddEffectEvent({
		World::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), 1.25f });
}

void Spinner::ShootRadialVolley()
{
	for (std::size_t i{ 0 }; i < GetWeaponEmitters().size(); ++i)
	{
		const sf::Vector2f emitter{ GetWeaponEmitterPosition(i) };
		const sf::Vector2f direction{ Normalize(emitter - GetPosition()) };
		GetWorld().SpawnSaucerShot(
			emitter,
			emitter + direction * 100.f,
			GameplayData::ProjectileKind::Spinner,
			i == 0);
	}
}

sf::Vector2f Spinner::GetWeaponEmitterPosition(std::size_t index) const
{
	const sf::Sprite& sprite{ GetSprite() };
	const sf::IntRect textureRect{ sprite.getTextureRect() };
	const GameplayData::NormalizedPoint& emitter{ GetWeaponEmitters().at(index) };
	const sf::Vector2f localPosition{
		static_cast<float>(textureRect.position.x) +
			static_cast<float>(textureRect.size.x) * emitter.x,
		static_cast<float>(textureRect.position.y) +
			static_cast<float>(textureRect.size.y) * emitter.y };
	return sprite.getTransform().transformPoint(localPosition);
}
