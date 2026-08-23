#include "Saucer.h"

#include <cmath>
#include <numbers>
#include "utils/Random.h"
#include "utils/ConfigEnums.h"
#include "assets/Assets.h"
#include "core/World.h"

Saucer::Saucer(Assets& assets, World& world, Mode mode)
	: Enemy(assets, world, GetTexture(assets, mode), GetConfig(assets, mode))
	, mode(mode)
{
	spinDirection = Random::Float(0.f, 1.f) < 0.5f ? -1.f : 1.f;
}

Entity::Type Saucer::GetType() const noexcept
{
	return Type::Enemy;
}

bool Saucer::IsCollideWith(const Entity& other) const
{
	if (other.GetType() != Type::Player &&
		other.GetType() != Type::Projectile_Player &&
		other.GetType() != Type::Projectile_Ally &&
		other.GetType() != Type::EnemyMissile)
		return false;

	return CheckCollision(other);
}

void Saucer::Update(float deltaTime)
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
		const float progress{ 1.f - materializationRemaining / materializationDuration };
		SetPresentation(0.18f + 0.82f * progress, progress,
			sf::Color(255, 80, 80));
		return;
	}
	const sf::Vector2f playerPos{ GetWorld().GetPlayerPosition() };
	const sf::Vector2f toPlayer{ playerPos - GetPosition() };

	if (mode == Mode::Kamikaze)
	{
		SetRotation(GetRotation() + sf::degrees(
			GetRotationSpeed() * spinDirection * deltaTime));
		UpdateMovement(deltaTime, playerPos);
	}
	else
	{
		TurnTowards(playerPos, GetRotationSpeed(), deltaTime);

		if (approachingCenter)
		{
			if (MoveToTarget(deltaTime, approachTarget))
			{
				approachingCenter = false;
				hasPatrolTarget = false;
			}
		}
		else
		{
			if (!hasPatrolTarget)
				ChooseCentralPatrolTarget();
			if (MoveToTarget(deltaTime, patrolTarget))
				ChooseCentralPatrolTarget();
		}
	}

	if (mode == Mode::Shooter)
	{
		shootTimer += deltaTime;

		if (shootTimer >= GetActionInterval())
			Shoot(playerPos);
	}
}

void Saucer::OnDestroy()
{
	GetWorld().AddSound(Config::Sound::ShipExplosion);
	GetWorld().AddEffectEvent({
		World::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), mode == Mode::Shooter ? 1.1f : 0.95f });
}

Saucer::Mode Saucer::GetMode() const noexcept
{
	return mode;
}

void Saucer::BeginMaterialization(float duration, const Entity* anchor) noexcept
{
	materializationDuration = std::max(0.05f, duration);
	materializationRemaining = materializationDuration;
	materializationAnchor = anchor;
	SetVelocity({});
	SetPresentation(0.18f, 0.f, sf::Color(255, 80, 80));
}

void Saucer::ConfigureApproachTarget(sf::Vector2f target) noexcept
{
	approachTarget = target;
	approachingCenter = true;
	hasPatrolTarget = false;
}

bool Saucer::MoveToTarget(float deltaTime, const sf::Vector2f& target)
{
	const sf::Vector2f delta{ target - GetPosition() };
	const float distance{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
	const float step{ GetMovementSpeed() * deltaTime };
	if (distance <= std::max(step, 0.001f))
	{
		SetPosition(target);
		SetVelocity({});
		return true;
	}
	SetVelocity(delta / distance * GetMovementSpeed());
	Move(deltaTime);
	return false;
}

void Saucer::ChooseCentralPatrolTarget()
{
	const float width{ static_cast<float>(GetWorld().GetWidth()) };
	const float height{ static_cast<float>(GetWorld().GetHeight()) };
	patrolTarget = {
		Random::Float(width * 0.22f, width * 0.78f),
		Random::Float(height * 0.2f, height * 0.8f) };
	hasPatrolTarget = true;
}

void Saucer::UpdateMovement(float deltaTime, const sf::Vector2f& target)
{
	const sf::Vector2f toTarget{ target - GetPosition() };
	const float angle{ std::atan2(toTarget.y, toTarget.x) };
	const sf::Vector2f direction{ std::cos(angle),std::sin(angle) };

	SetVelocity(direction * GetMovementSpeed());
	Move(deltaTime);
}

void Saucer::Shoot(const sf::Vector2f& playerPosition)
{
	(void)playerPosition;
	shootTimer -= GetActionInterval();
	const sf::Vector2f muzzle{ GetWeaponEmitterPosition(nextWeaponEmitter) };
	GetWorld().SpawnSaucerShot(
		muzzle, muzzle + GetForwardDirection() * 1000.f);
	nextWeaponEmitter = (nextWeaponEmitter + 1) % GetWeaponEmitters().size();
}

sf::Vector2f Saucer::GetWeaponEmitterPosition(std::size_t index) const
{
	const sf::Sprite& entitySprite{ GetSprite() };
	const sf::IntRect textureRect{ entitySprite.getTextureRect() };
	const GameplayData::NormalizedPoint& emitter{ GetWeaponEmitters().at(index) };
	const sf::Vector2f localPosition{
		static_cast<float>(textureRect.position.x) +
			static_cast<float>(textureRect.size.x) * emitter.x,
		static_cast<float>(textureRect.position.y) +
			static_cast<float>(textureRect.size.y) * emitter.y };
	return entitySprite.getTransform().transformPoint(localPosition);
}

const GameplayData::EnemyConfig& Saucer::GetConfig(Assets& assets, Mode mode)
{
	using Kind = GameplayData::EnemyKind;
	switch (mode)
	{
	case Mode::Kamikaze:
		return assets.GetGameplayData().GetEnemy(Kind::Kamikaze);
	case Mode::Shooter:
		return assets.GetGameplayData().GetEnemy(Kind::Shooter);
	default:
		std::unreachable();
	}
}

sf::Texture& Saucer::GetTexture(Assets& assets, Mode mode)
{
	switch (mode)
	{
		using Texture = Config::Texture;

	case Mode::Kamikaze:
		return assets.Textures().Get(Texture::BigEnemySaucer);

	case Mode::Shooter:
		return assets.Textures().Get(Texture::SmallEnemySaucer);

	default:
		return assets.Textures().Get(Texture::BigEnemySaucer);
	}
}
