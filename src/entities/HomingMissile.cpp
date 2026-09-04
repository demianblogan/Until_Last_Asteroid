#include "HomingMissile.h"

#include <algorithm>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/VectorMath.h"

HomingMissile::HomingMissile(Assets& assets, World& world, const sf::Vector2f& position, const sf::Vector2f& target)
	: Entity(assets, world,
		assets.Textures().Get(Config::Texture::HomingMissile),
		assets.GetGameplayData().GetMissile().visualScale,
		assets.GetGameplayData().GetMissile().collisionRadius)
	, health(assets.GetGameplayData().GetMissile().maximumHealth)
{
	const auto& config = assets.GetGameplayData().GetMissile();

	speed = config.speed;
	turnSpeedDegrees = config.turnSpeedDegrees;
	explosionRadius = config.explosionRadius;
	explosionImpulse = config.explosionImpulse;
	lifetimeRemaining = config.lifetime;
	explosionDamage = config.explosionDamage;

	SetPosition(position);

	const sf::Vector2f direction{ VectorMath::Normalize(target - position, { 0.f, -1.f }) };
	SetVelocity(direction * speed);
	// Sprite art points up at rotation 0, so facing = heading angle + 90.
	SetRotation(direction.angle() + sf::degrees(90.f));
}

Entity::Type HomingMissile::GetType() const noexcept
{
	return Type::EnemyMissile;
}

bool HomingMissile::IsCollidingWith(const Entity& other) const
{
	const Type type = other.GetType();

	return (type == Type::Player || type == Type::Enemy ||
		type == Type::Asteroid || type == Type::Projectile_Player ||
		type == Type::Projectile_Ally ||
		type == Type::EnemyMissile) && CheckCollision(other);
}

void HomingMissile::Update(float deltaTime)
{
	lifetimeRemaining -= deltaTime;

	if (lifetimeRemaining <= 0.f)
	{
		Destroy();
		return;
	}

	// Turn the current heading toward the player by a bounded amount this
	// frame (a gradual chase, not an instant lock), keeping speed constant.
	const sf::Vector2f direction{ VectorMath::RotateToward(
		GetVelocity(),
		GetWorld().GetPlayerPosition() - GetPosition(),
		sf::degrees(turnSpeedDegrees * deltaTime),
		GetForwardDirection()) };

	SetVelocity(direction * speed);
	SetRotation(direction.angle() + sf::degrees(90.f));
	Move(deltaTime);

	GetWorld().Effects().Add({ Rendering::EffectEventType::MissileSmoke,	GetPosition(), direction });
	GetWorld().Effects().Add({ Rendering::EffectEventType::EnemyProjectileGlow,GetPosition(), direction, 1.2f });

	constexpr float DespawnMargin = 160.f;
	const sf::Vector2f position = GetPosition();

	if (position.x < -DespawnMargin ||
		position.y < -DespawnMargin ||
		position.x > static_cast<float>(GetWorld().GetWidth()) + DespawnMargin ||
		position.y > static_cast<float>(GetWorld().GetHeight()) + DespawnMargin)
	{
		Destroy();
	}
}

void HomingMissile::OnDestroy()
{
	if (!isDetonating)
		return;

	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion, 1.15f);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPosition(), GetVelocity(), 0.72f });
	GetWorld().ExplodeEnemyMissile(GetPosition(), explosionRadius, explosionDamage, explosionImpulse);
}

bool HomingMissile::TakeDamage(int damage)
{
	if (!health.ApplyDamage(damage))
		return false;

	if (!health.IsDepleted())
	{
		FlashOnHit(GetAssets().GetGameplayData().GetHitFlashDuration());
		return false;
	}

	Detonate();

	return true;
}

void HomingMissile::Detonate()
{
	if (!IsAlive())
		return;

	isDetonating = true;

	Destroy();
}