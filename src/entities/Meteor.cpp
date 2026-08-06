#include "Meteor.h"

#include <array>
#include <cmath>
#include <numbers>
#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "core/World.h"

Meteor::Meteor(AssetStore& assets, World& world, Size size)
	: Enemy(assets, world, GetRandomTexture(assets, size), GetScore(size), GetSpeed(size)), size(size)
{
	// No code
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
	switch (size)
	{
	case Size::Small:
		GetWorld().AddSound(Config::Sound::SmallMeteorExplosion);
		break;

	case Size::Medium:
		GetWorld().AddSound(Config::Sound::MediumMeteorExplosion);
		break;

	case Size::Big:
		GetWorld().AddSound(Config::Sound::BigMeteorExplosion);
		break;
	}

	Size newSize;

	switch (size)
	{
	case Size::Big:
		newSize = Size::Medium;
		break;
	case Size::Medium:
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

		static constexpr float FRAGMENT_SPEED{ 150.f };
		meteor->SetVelocity(direction * FRAGMENT_SPEED);

		GetWorld().Spawn(std::move(meteor));
	}
}

float Meteor::GetSpeed(Meteor::Size size) noexcept
{
	switch (size)
	{
	case Size::Small:
		return 300.f;
	case Size::Medium:
		return 200.f;
	case Size::Big:
		return 100.f;

	default:
		std::unreachable();
	}
}

int Meteor::GetScore(Meteor::Size size) noexcept
{
	switch (size)
	{
	case Size::Small:
		return 100;
	case Size::Medium:
		return 60;
	case Size::Big:
		return 20;

	default:
		std::unreachable();
	}
}

sf::Texture& Meteor::GetRandomTexture(AssetStore& assets, Meteor::Size size)
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

	case Size::Medium:
	{
		std::array medium
		{
			Texture::MediumMeteor1, Texture::MediumMeteor2
		};

		return assets.Textures().Get(medium[Random::Int(0, 1)]);
	}

	case Size::Big:
	{
		std::array big
		{
			Texture::BigMeteor1, Texture::BigMeteor2,
			Texture::BigMeteor3, Texture::BigMeteor4
		};

		return assets.Textures().Get(big[Random::Int(0, 3)]);
	}

	default:
		return assets.Textures().Get(Texture::BigMeteor1);
	}
}
