#include "Meteor.h"

#include <array>
#include <utility>

#include "assets/Assets.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "utils/VectorMath.h"
#include "core/world/World.h"

Meteor::Meteor(Assets& assets, World& world, Size size)
	: Enemy(assets, world, GetRandomTexture(assets, size), GetConfig(assets, size))
	, size(size)
	, fragmentSpeed(GetConfig(assets, size).fragmentSpeed)
{
	const float direction = Random::Int(0, 1) == 0 ? -1.f : 1.f;
	angularVelocity = GetRotationSpeed() * Random::Float(0.65f, 1.35f) * direction;
	SetRotation(sf::degrees(Random::Float(0.f, 360.f)));

	// Meteor never sets its own velocity again after this -- unlike every
	// other Enemy, it just drifts in a straight line for its whole life, so
	// (unlike the other subclasses) this initial heading actually matters and
	// is picked as a uniformly random direction around the full circle.
	SetVelocity(VectorMath::RandomDirection() * GetMovementSpeed());
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

bool Meteor::IsCollidingWith(const Entity& other) const
{
	if (other.GetType() == Type::Asteroid)
		return false;

	return CheckCollision(other);
}

void Meteor::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::AsteroidExplosion, GetSoundPitchMultiplier());
	GetWorld().Effects().Add({
		Rendering::EffectEventType::AsteroidExplosion,
		GetPosition(),
		GetVelocity(),
		size == Size::Big ? 1.f : 0.62f });

	if (!isFragmentSpawningEnabled)
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

	static constexpr int FragmentCount = 2;
	for (int i = 0; i < FragmentCount; i++)
	{
		auto meteor = std::make_unique<Meteor>(GetAssets(), GetWorld(), newSize);

		meteor->SetPosition(GetPosition());
		meteor->SetVelocity(VectorMath::RandomDirection() * fragmentSpeed);

		GetWorld().Spawn(std::move(meteor));
	}
}

const GameplayData::EnemyConfig& Meteor::GetConfig(Assets& assets, Meteor::Size size)
{
	switch (size)
	{
		using Kind = GameplayData::EnemyKind;

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

void Meteor::SetFragmentSpawningEnabled(bool isEnabled) noexcept
{
	isFragmentSpawningEnabled = isEnabled;
}

sf::Texture& Meteor::GetRandomTexture(Assets& assets, Meteor::Size size)
{
	using Texture = Config::Texture;

	static constexpr std::array SmallTextures
	{
		Texture::SmallMeteor1,
		Texture::SmallMeteor2,
		Texture::SmallMeteor3,
		Texture::SmallMeteor4
	};

	static constexpr std::array BigTextures
	{
		Texture::BigMeteor1,
		Texture::BigMeteor2,
		Texture::BigMeteor3,
		Texture::BigMeteor4
	};

	switch (size)
	{
	case Size::Small:
		return assets.Textures().Get(SmallTextures[Random::Int(0, static_cast<int>(SmallTextures.size()) - 1)]);

	case Size::Big:
		return assets.Textures().Get(BigTextures[Random::Int(0, static_cast<int>(BigTextures.size()) - 1)]);

	default:
		std::unreachable();
	}
}
