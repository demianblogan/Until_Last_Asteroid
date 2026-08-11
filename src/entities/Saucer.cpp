#include "Saucer.h"

#include <cmath>
#include <numbers>
#include "utils/Random.h"
#include "utils/ConfigEnums.h"
#include "assets/AssetStore.h"
#include "core/World.h"

Saucer::Saucer(AssetStore& assets, World& world, Mode mode)
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
	if (other.GetType() != Type::Player && other.GetType() != Type::Projectile_Player)
		return false;

	return CheckCollision(other);
}

void Saucer::Update(float deltaTime)
{
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
		const float angleRad{ std::atan2(toPlayer.y, toPlayer.x) };
		const float angleDeg{ angleRad * 180.f / std::numbers::pi_v<float> };
		static constexpr float RotationOffset{ 90.f };
		SetRotation(sf::degrees(angleDeg + RotationOffset));

		if (GetVelocity().x == 0.f && GetVelocity().y == 0.f)
		{
			float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
			sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
			SetVelocity(direction * GetMovementSpeed());
		}

		Move(deltaTime);
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
	shootTimer -= GetActionInterval();
	GetWorld().SpawnSaucerShot(
		GetWeaponEmitterPosition(nextWeaponEmitter), playerPosition);
	nextWeaponEmitter = (nextWeaponEmitter + 1) % GetWeaponEmitters().size();
}

sf::Vector2f Saucer::GetWeaponEmitterPosition(std::size_t index) const
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

const GameplayData::EnemyConfig& Saucer::GetConfig(AssetStore& assets, Mode mode)
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

sf::Texture& Saucer::GetTexture(AssetStore& assets, Mode mode)
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
