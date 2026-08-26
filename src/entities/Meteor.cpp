#include "Meteor.h"

#include <array>
#include <cmath>
#include <numbers>
#include "assets/Assets.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "core/world/World.h"

Meteor::Meteor(Assets& assets, World& world, Size size)
	: Enemy(assets, world, GetRandomTexture(assets, size), GetConfig(assets, size)), size(size)
{
	const float direction{ Random::Int(0, 1) == 0 ? -1.f : 1.f };
	angularVelocity = GetRotationSpeed() * Random::Float(0.65f, 1.35f) * direction;
	SetRotation(sf::degrees(Random::Float(0.f, 360.f)));
}

void Meteor::Update(float deltaTime)
{
	Enemy::Update(deltaTime);
	SetRotation(GetRotation() + sf::degrees(angularVelocity * deltaTime));
}

Entity::Type Meteor::GetType() const noexcept
{
	return Type::Asteroid;
}

bool Meteor::IsCollideWith(const Entity& other) const
{
	if (other.GetType() == Type::Asteroid)
		return false;

	return CheckCollision(other);
}

void Meteor::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::AsteroidExplosion, GetSoundPitch());
	GetWorld().Effects().Add({
		Rendering::EffectEventType::AsteroidExplosion,
		GetPosition(),
		GetVelocity(),
		size == Size::Big ? 1.f : 0.62f });

	if (!fragmentSpawningEnabled)
		return;

	Size newSize;

	switch (size)
	{
	case Size::Big:
		newSize = Size::Small;
		break;
	case Size::Small:
		return; // small ones are not dividable
	}

	static constexpr int FRAGMENT_COUNT{ 2 };
	for (int i{ 0 }; i < FRAGMENT_COUNT; i++)
	{
		auto meteor = std::make_unique<Meteor>(GetAssets(), GetWorld(), newSize);

		meteor->SetPosition(GetPosition());

		const float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
		sf::Vector2f direction{ std::cos(angle), std::sin(angle) };

		meteor->SetVelocity(direction * GetFragmentSpeed());

		GetWorld().Spawn(std::move(meteor));
	}
}

const GameplayData::EnemyConfig& Meteor::GetConfig(Assets& assets, Meteor::Size size)
{
	using Kind = GameplayData::EnemyKind;
	switch (size)
	{
	case Size::Small:
		return assets.GetGameplayData().GetEnemy(Kind::SmallMeteor);
	case Size::Big:
		return assets.GetGameplayData().GetEnemy(Kind::BigMeteor);

	default:
		std::unreachable();
	}
}

Meteor::Size Meteor::GetSize() const noexcept
{
	return size;
}

void Meteor::SetFragmentSpawningEnabled(bool enabled) noexcept
{
	fragmentSpawningEnabled = enabled;
}

sf::Texture& Meteor::GetRandomTexture(Assets& assets, Meteor::Size size)
{
	using Texture = Config::Texture;

	switch (size)
	{
	case Size::Small:
	{
		std::array small
		{
			Texture::SmallMeteor1,
			Texture::SmallMeteor2,
			Texture::SmallMeteor3,
			Texture::SmallMeteor4
		};

		return assets.Textures().Get(small[Random::Int(0, static_cast<int>(small.size()) - 1)]);
	}

	case Size::Big:
	{
		std::array big
		{
			Texture::BigMeteor1, Texture::BigMeteor2,
			Texture::BigMeteor3, Texture::BigMeteor4
		};

		return assets.Textures().Get(
			big[Random::Int(0, static_cast<int>(big.size()) - 1)]);
	}

	default:
		return assets.Textures().Get(Texture::BigMeteor1);
	}
}
