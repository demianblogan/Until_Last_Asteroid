#include "Saucer.h"

#include <cmath>
#include <numbers>
#include "utils/Random.h"
#include "utils/ConfigEnums.h"
#include "assets/AssetStore.h"
#include "core/World.h"

Saucer::Saucer(AssetStore& assets, World& world, Mode mode)
	: Enemy(assets, world, GetTexture(assets, mode), GetScore(mode), GetSpeed(mode))
	, mode(mode)
{
	// No code
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

	const float angleRad{ std::atan2(toPlayer.y, toPlayer.x) };
	const float angleDeg{ angleRad * 180.f / std::numbers::pi_v<float> };

	static constexpr float ROTATION_OFFSET{ 90.f };
	SetRotation(sf::degrees(angleDeg + ROTATION_OFFSET));

	if (mode == Mode::Kamikaze)
	{
		UpdateMovement(deltaTime, playerPos);
	}
	else
	{
		if (GetVelocity().x == 0.f && GetVelocity().y == 0.f)
		{
			float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
			sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
			SetVelocity(direction * GetSpeed(mode));
		}

		Move(deltaTime);
	}

	if (mode == Mode::Shooter)
	{
		static constexpr float SHOOT_INTERVAL = 1.5f;

		shootTimer += deltaTime;

		if (shootTimer > SHOOT_INTERVAL)
			Shoot(playerPos);
	}
}

void Saucer::OnDestroy()
{
	GetWorld().AddSound(Config::Sound::EnemySaucerExplosion);
}

void Saucer::UpdateMovement(float deltaTime, const sf::Vector2f& target)
{
	const sf::Vector2f toTarget{ target - GetPosition() };
	const float angle{ std::atan2(toTarget.y, toTarget.x) };
	const sf::Vector2f direction{ std::cos(angle),std::sin(angle) };

	SetVelocity(direction * GetSpeed(mode));
	Move(deltaTime);
}

void Saucer::Shoot(const sf::Vector2f& playerPosition)
{
	shootTimer = 0.f;

	GetWorld().SpawnSaucerShot(GetPosition(), playerPosition);
	GetWorld().AddSound(Config::Sound::EnemyLaserShot);
}

float Saucer::GetSpeed(Mode mode) noexcept
{
	switch (mode)
	{
	case Mode::Kamikaze:
		return 500.f;
	case Mode::Shooter:
		return 200.f;
	default:
		return 0.f;
	}
}

int Saucer::GetScore(Mode mode) noexcept
{
	switch (mode)
	{
	case Mode::Kamikaze:
		return 50;
	case Mode::Shooter:
		return 200;
	default:
		return 0;
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