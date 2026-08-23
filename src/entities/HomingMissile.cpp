#include "HomingMissile.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include "assets/Assets.h"
#include "core/World.h"
#include "utils/ConfigEnums.h"

HomingMissile::HomingMissile(
	Assets& assets,
	World& world,
	const sf::Vector2f& position,
	const sf::Vector2f& target)
	: Entity(
		assets,
		world,
		assets.Textures().Get(Config::Texture::HomingMissile),
		assets.GetGameplayData().GetMissile().visualScale,
		assets.GetGameplayData().GetMissile().collisionRadius)
	, health(assets.GetGameplayData().GetMissile().maximumHealth)
{
	const auto& config{ assets.GetGameplayData().GetMissile() };
	speed = config.speed;
	turnSpeedRadians = config.turnSpeedDegrees *
		std::numbers::pi_v<float> / 180.f;
	explosionRadius = config.explosionRadius;
	explosionImpulse = config.explosionImpulse;
	lifetimeRemaining = config.lifetime;
	explosionDamage = config.explosionDamage;

	SetPosition(position);
	const sf::Vector2f toTarget{ target - position };
	const float angle{ std::atan2(toTarget.y, toTarget.x) };
	SetVelocity({ std::cos(angle) * speed, std::sin(angle) * speed });
	SetRotation(sf::radians(angle + std::numbers::pi_v<float> * 0.5f));
}

Entity::Type HomingMissile::GetType() const noexcept
{
	return Type::EnemyMissile;
}

bool HomingMissile::IsCollideWith(const Entity& other) const
{
	const Type type{ other.GetType() };
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

	const sf::Vector2f currentVelocity{ GetVelocity() };
	const sf::Vector2f toPlayer{ GetWorld().GetPlayerPosition() - GetPosition() };
	const float currentAngle{ std::atan2(currentVelocity.y, currentVelocity.x) };
	const float targetAngle{ std::atan2(toPlayer.y, toPlayer.x) };
	const float angleDifference{ std::atan2(
		std::sin(targetAngle - currentAngle),
		std::cos(targetAngle - currentAngle)) };
	const float maximumTurn{ turnSpeedRadians * deltaTime };
	const float finalAngle{ currentAngle + std::clamp(
		angleDifference, -maximumTurn, maximumTurn) };
	const sf::Vector2f direction{ std::cos(finalAngle), std::sin(finalAngle) };
	SetVelocity(direction * speed);
	SetRotation(sf::radians(finalAngle + std::numbers::pi_v<float> * 0.5f));
	Move(deltaTime);

	GetWorld().AddEffectEvent({
		World::EffectEventType::MissileSmoke,
		GetPosition(), direction });
	GetWorld().AddEffectEvent({
		World::EffectEventType::EnemyProjectileGlow,
		GetPosition(), direction, 1.2f });

	constexpr float DespawnMargin{ 160.f };
	const sf::Vector2f position{ GetPosition() };
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
	if (!detonating)
		return;

	GetWorld().AddSound(Config::Sound::ShipExplosion, 1.15f);
	GetWorld().AddEffectEvent({
		World::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), 0.72f });
	GetWorld().ExplodeEnemyMissile(
		GetPosition(), explosionRadius, explosionDamage, explosionImpulse);
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
	detonating = true;
	Destroy();
}
