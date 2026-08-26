#include "World.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <utility>
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "entities/Enemy.h"
#include "entities/HelperBot.h"
#include "entities/HomingMissile.h"
#include "entities/LaserTurret.h"
#include "entities/Meteor.h"
#include "entities/Part.h"
#include "entities/Player.h"
#include "entities/Pickup.h"
#include "entities/ReflectorGunship.h"
#include "entities/Saucer.h"
#include "entities/ShooterStation.h"
#include "entities/Shot.h"
#include "gameplay/GameplaySession.h"
#include "rendering/EnergyShield.h"
#include "core/Collision.h"
#include "core/world/ProjectileGeometry.h"
#include "input/GamepadManager.h"

namespace
{
	sf::Vector2f Normalize(const sf::Vector2f& vector)
	{
		const float lengthSquared{ vector.x * vector.x + vector.y * vector.y };
		if (lengthSquared <= 0.0001f)
			return { 1.f, 0.f };
		return vector / std::sqrt(lengthSquared);
	}

	void DrawPickupAura(
		sf::RenderTarget& target,
		const Pickup& pickup,
		float visualTime,
		sf::RenderStates states)
	{
		sf::Color color;
		switch (pickup.GetKind())
		{
		case Pickup::Kind::Health:
			color = sf::Color(75, 255, 105);
			break;
		case Pickup::Kind::Shield:
			color = sf::Color(45, 230, 255);
			break;
		case Pickup::Kind::HomingBullets:
			color = sf::Color(255, 188, 45);
			break;
		case Pickup::Kind::TimeSlowdown:
			color = sf::Color(180, 75, 255);
			break;
		case Pickup::Kind::Laser:
			color = sf::Color(255, 48, 25);
			break;
		case Pickup::Kind::TripleShot:
			color = sf::Color(255, 175, 28);
			break;
		case Pickup::Kind::HelperBot:
			color = sf::Color(70, 240, 255);
			break;
		}
		const float pulse{ 0.78f + 0.22f * std::sin(visualTime * 4.5f) };
		states.blendMode = sf::BlendAdd;

		for (int layer{ 0 }; layer < 3; ++layer)
		{
			const float radius{ pickup.GetCollisionRadius() * (1.15f + layer * 0.23f) };
			sf::CircleShape aura(radius, 48u);
			aura.setOrigin({ radius, radius });
			aura.setPosition(pickup.GetPosition());
			const auto alpha{ static_cast<std::uint8_t>((34.f - layer * 8.f) * pulse) };
			aura.setFillColor(sf::Color(color.r, color.g, color.b, alpha));
			target.draw(aura, states);
		}
	}

	void DrawPartAura(
		sf::RenderTarget& target,
		const Part& part,
		float visualTime,
		sf::RenderStates states)
	{
		const sf::Color color{ 255, 184, 38 };
		const float pulse{ 0.55f + 0.45f * std::abs(
			std::sin(visualTime * 2.f * std::numbers::pi_v<float>)) };
		states.blendMode = sf::BlendAdd;
		for (int layer{ 0 }; layer < 3; ++layer)
		{
			const float radius{ part.GetCollisionRadius() * (1.1f + layer * 0.24f) };
			sf::CircleShape aura(radius, 48u);
			aura.setOrigin({ radius, radius });
			aura.setPosition(part.GetPosition());
			const auto alpha{ static_cast<std::uint8_t>((36.f - layer * 9.f) * pulse) };
			aura.setFillColor(sf::Color(color.r, color.g, color.b, alpha));
			target.draw(aura, states);
		}
	}

	void DrawShieldOverlay(
		sf::RenderTarget& target,
		const Player& player,
		float visualTime,
		float ratio,
		float hitFlashRatio,
		sf::RenderStates states)
	{
		float pulse{ 0.9f + 0.1f * std::sin(visualTime * 3.5f) };
		if (ratio <= 0.25f)
			pulse = 0.35f + 0.65f * std::abs(std::sin(visualTime * 14.f));
		if (hitFlashRatio > 0.f)
		{
			const float hitBlink{ 0.18f +
				0.82f * std::abs(std::sin(hitFlashRatio * 8.f * std::numbers::pi_v<float>)) };
			pulse = std::max(pulse, hitBlink * (1.f + hitFlashRatio * 0.55f));
		}

		Rendering::DrawEnergyShield(target, player.GetPosition(),
			player.GetCollisionRadius() * 1.62f, pulse,
			{ 20, 205, 235 }, { 70, 240, 255 }, { 45, 225, 255 }, 24.f, states);
	}

	void DrawFeatheredBeam(
		sf::RenderTarget& target,
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		float beamWidth,
		float pulse,
		sf::Color coreColor,
		sf::Color glowColor,
		sf::RenderStates states)
	{
		const sf::Vector2f delta{ end - start };
		const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
		if (length <= 0.1f)
			return;

		const sf::Vector2f direction{ delta / length };
		const sf::Vector2f perpendicular{ -direction.y, direction.x };
		const float outerHalfWidth{ beamWidth * 2.8f };
		const float glowHalfWidth{ beamWidth * 1.35f };
		const float coreHalfWidth{ beamWidth * 0.5f };
		auto withAlpha = [pulse](sf::Color color, float alpha)
		{
			color.a = static_cast<std::uint8_t>(
				std::clamp(alpha * pulse, 0.f, 255.f));
			return color;
		};

		const std::array offsets{
			-outerHalfWidth, -glowHalfWidth, -coreHalfWidth,
			0.f,
			coreHalfWidth, glowHalfWidth, outerHalfWidth };
		const std::array colors{
			withAlpha(glowColor, 0.f),
			withAlpha(glowColor, 38.f),
			withAlpha(glowColor, 150.f),
			withAlpha(coreColor, 255.f),
			withAlpha(glowColor, 150.f),
			withAlpha(glowColor, 38.f),
			withAlpha(glowColor, 0.f) };

		sf::VertexArray beam(sf::PrimitiveType::TriangleStrip);
		beam.resize(offsets.size() * 2u);
		for (std::size_t index{ 0u }; index < offsets.size(); ++index)
		{
			beam[index * 2u] = sf::Vertex{
				start + perpendicular * offsets[index], colors[index] };
			beam[index * 2u + 1u] = sf::Vertex{
				end + perpendicular * offsets[index], colors[index] };
		}

		states.blendMode = sf::BlendAdd;
		target.draw(beam, states);
	}

	void DrawLaserBeam(
		sf::RenderTarget& target,
		const LaserTurret& turret,
		sf::RenderStates states)
	{
		if (!turret.IsBeamActive())
			return;
		const sf::Vector2f start{ turret.GetBeamStart() };
		const sf::Vector2f delta{ turret.GetBeamEnd() - start };
		const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
		if (length <= 0.1f)
			return;
		const float pulse{ turret.GetBeamPulse() };
		DrawFeatheredBeam(target, start, turret.GetBeamEnd(),
			turret.GetBeamWidth(), pulse,
			sf::Color(255, 238, 210), sf::Color(255, 28, 10), states);
		states.blendMode = sf::BlendAdd;

		const sf::Vector2f direction{ delta / length };
		const sf::Vector2f perpendicular{ -direction.y, direction.x };
		constexpr int MarkerCount{ 18 };
		const float animationTime{ turret.GetBeamAnimationTime() };
		for (int index{ 0 }; index < MarkerCount; ++index)
		{
			const float base{ static_cast<float>(index) / MarkerCount };
			const float travel{ std::fmod(base + animationTime * 0.7f, 1.f) };
			const float phase{ travel * 4.f * std::numbers::pi_v<float> +
				animationTime * 5.f };
			const float offset{ std::sin(phase) * turret.GetBeamWidth() * 0.8f };
			const float depth{ 0.5f + 0.5f * std::cos(phase) };
			const float radius{ 3.f + depth * 4.f };
			sf::CircleShape diamond(radius, 4u);
			diamond.setOrigin({ radius, radius });
			diamond.setRotation(sf::degrees(45.f + animationTime * 150.f));
			diamond.setPosition(start + direction * (travel * length) +
				perpendicular * offset);
			diamond.setFillColor(sf::Color(255, 210, 125,
				static_cast<std::uint8_t>(135.f + depth * 120.f)));
			target.draw(diamond, states);
		}
	}

	void DrawPlayerLaser(
		sf::RenderTarget& target,
		const Player& player,
		float beamWidth,
		sf::RenderStates states)
	{
		if (!player.IsLaserFiring())
			return;
		const sf::Vector2f start{ player.GetPosition() };
		const sf::Vector2f delta{ player.GetLaserEndPosition() - start };
		const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
		if (length <= 0.1f)
			return;
		const float time{ player.GetLaserVisualTime() };
		const float pulse{ 0.82f + 0.18f * std::sin(time * 18.f) };
		DrawFeatheredBeam(target, start, player.GetLaserEndPosition(),
			beamWidth, pulse,
			sf::Color(225, 255, 255), sf::Color(20, 175, 255), states);
		states.blendMode = sf::BlendAdd;

		const sf::Vector2f direction{ delta / length };
		const sf::Vector2f perpendicular{ -direction.y, direction.x };
		for (int index{ 0 }; index < 22; ++index)
		{
			const float base{ static_cast<float>(index) / 22.f };
			// Advance from the ship toward the far end of the beam.
			const float travel{ std::fmod((1.f - base) + time * 1.25f, 1.f) };
			const float side{ std::sin(travel * 6.f * std::numbers::pi_v<float> + time * 9.f) };
			const float radius{ 2.5f + 2.f * std::abs(side) };
			sf::CircleShape spark(radius, 4u);
			spark.setOrigin({ radius, radius });
			spark.setRotation(sf::degrees(45.f + time * 210.f));
			spark.setPosition(start + direction * (travel * length) +
				perpendicular * (side * beamWidth * 0.7f));
			spark.setFillColor(sf::Color(120, 235, 255, 210));
			target.draw(spark, states);
		}
	}

	void DrawStationEffects(
		sf::RenderTarget& target,
		const ShooterStation& station,
		sf::RenderStates states)
	{
		states.blendMode = sf::BlendAdd;
		const float charge{ station.GetSpawnChargeRatio() };
		if (charge > 0.f)
		{
			const float radius{ 18.f + charge * 30.f };
			sf::CircleShape portal(radius, 64u);
			portal.setOrigin({ radius, radius });
			portal.setPosition(station.GetPosition());
			portal.setFillColor(sf::Color(255, 28, 18,
				static_cast<std::uint8_t>(70.f * charge)));
			portal.setOutlineColor(sf::Color(255, 120, 55,
				static_cast<std::uint8_t>(210.f * charge)));
			portal.setOutlineThickness(4.f + 5.f * charge);
			target.draw(portal, states);
		}

		if (!station.IsShieldActive())
			return;
		Rendering::DrawEnergyShield(target, station.GetPosition(),
			station.GetShieldRadius(), station.GetShieldPulse(),
			{ 220, 28, 38 }, { 255, 78, 62 }, { 255, 40, 35 }, 64.f, states);
	}
}

World::World(unsigned int width, unsigned int height, Assets& assets,
	AudioManager& audio, GameplaySession& session, GamepadManager& gamepadManager)
	: assets(assets), session(session), gamepad(gamepadManager),
	  sound(audio), width(width), height(height)
{
}

void World::Update(float deltaTime, float worldTimeScale)
{
	playerLaserDamageEvent.reset();
	shieldVisualTime += deltaTime;
	worldTimeScale = std::clamp(worldTimeScale, 0.f, 1.f);
	CommitPendingEntities();

	for (auto& entity : entities)
	{
		const Entity::Type type{ entity->GetType() };
		const bool usesPlayerTime{
			type == Entity::Type::Player ||
			type == Entity::Type::Companion ||
			type == Entity::Type::Projectile_Player ||
			type == Entity::Type::Projectile_Ally ||
			type == Entity::Type::Pickup };
		const float entityDeltaTime{ usesPlayerTime
			? deltaTime
			: deltaTime * worldTimeScale };
		entity->UpdateEffects(entityDeltaTime);
		entity->Update(entityDeltaTime);
		if (!entity->IsArriving() &&
			entity->GetType() != Entity::Type::Projectile_Player &&
			entity->GetType() != Entity::Type::Projectile_Ally &&
			entity->GetType() != Entity::Type::Projectile_Enemy &&
			entity->GetType() != Entity::Type::EnemyMissile)
		{
			Wrap(*entity);
		}
	}

	HandleCollisions();
	RemoveDeadEntities();
}

void World::CommitPendingEntities()
{
	if (pendingEntities.empty())
		return;

	entities.insert(entities.end(),
		std::make_move_iterator(pendingEntities.begin()),
		std::make_move_iterator(pendingEntities.end()));
	pendingEntities.clear();
}

void World::Spawn(std::unique_ptr<Entity> entity)
{
	pendingEntities.push_back(std::move(entity));
}

void World::SpawnPlayer(
	Assets& playerAssets, InputHandler<Config::PlayerAction>& input, sf::RenderWindow& window)
{
	if (player != nullptr)
		return;

	auto playerPtr{ std::make_unique<Player>(playerAssets, *this, input, gamepad, window) };
	playerPtr->SetPosition({ GetWidth() * 0.5f, GetHeight() * 0.5f });
	player = playerPtr.get();
	Spawn(std::move(playerPtr));
}

void World::SetPlayerSpawnPresentation(float progress) noexcept
{
	if (player == nullptr)
		return;

	progress = std::clamp(progress, 0.f, 1.f);
	const float eased{ progress * progress * (3.f - 2.f * progress) };
	player->SetPresentation(0.38f + 0.62f * eased, eased);
}

void World::TeleportPlayerToCenter() noexcept
{
	if (player == nullptr)
		return;
	player->SetPosition({ GetWidth() * 0.5f, GetHeight() * 0.5f });
	player->SetVelocity({});
}

bool World::IsCleared() const noexcept
{
	const auto containsAliveEnemy = [](const auto& list)
	{
		return std::ranges::any_of(list, [](const auto& entity)
		{
			return entity->IsAlive() &&
				(entity->GetType() == Entity::Type::Enemy ||
				 entity->GetType() == Entity::Type::Asteroid ||
				 entity->GetType() == Entity::Type::EnemyMissile ||
				 entity->GetType() == Entity::Type::Part);
		});
	};

	return !containsAliveEnemy(entities) && !containsAliveEnemy(pendingEntities);
}

bool World::HasPlayer() const noexcept { return player != nullptr; }
Player* World::GetPlayer() const noexcept { return player; }

std::uint64_t World::BeginPlayerAttack() noexcept
{
	statisticsTracker.RecordAttackFired();
	return playerAttackTracker.Begin();
}

void World::RegisterPlayerAttackHit(std::uint64_t attackID) noexcept
{
	if (playerAttackTracker.RegisterHit(attackID))
		statisticsTracker.RecordAttackHit();
}

void World::SpawnPlayerShot(
	const sf::Vector2f& pos, float rotation, std::uint64_t attackID,
	bool playSound, bool tripleShotVisual)
{
	Spawn(std::make_unique<PlayerShot>(
		assets, *this, pos, rotation, attackID, playSound, tripleShotVisual));
}

void World::SpawnHelperShot(const sf::Vector2f& pos, const Entity* target)
{
	Spawn(std::make_unique<HelperShot>(assets, *this, pos, target));
}

bool World::SpawnHelperPickup(const sf::Vector2f& pos)
{
	if (helperPickupSpawned || session.IsHelperBotActive())
		return false;

	auto pickup{ std::make_unique<Pickup>(
		assets, *this, Pickup::Kind::HelperBot) };
	pickup->SetPosition(pos);
	Spawn(std::move(pickup));
	helperPickupSpawned = true;
	return true;
}

void World::SpawnPickupAt(GameplayData::PickupKind kind, const sf::Vector2f& pos)
{
	auto pickup{ std::make_unique<Pickup>(assets, *this, kind) };
	pickup->SetPosition(pos);
	Spawn(std::move(pickup));
}

void World::SpawnHelperBot()
{
	if (helperBotSpawned || !session.IsHelperBotActive())
		return;
	helperBotSpawned = true;
	Spawn(std::make_unique<HelperBot>(assets, *this));
}

void World::SpawnSaucerShot(
	const sf::Vector2f& pos,
	const sf::Vector2f& target,
	GameplayData::ProjectileKind projectileKind,
	bool playSound)
{
	Spawn(std::make_unique<SaucerShot>(
		assets, *this, pos, target, projectileKind, playSound));
}

void World::SpawnHomingMissile(const sf::Vector2f& pos, const sf::Vector2f& target)
{
	Spawn(std::make_unique<HomingMissile>(assets, *this, pos, target));
}

void World::SpawnStationShooter(
	const sf::Vector2f& position,
	float materializationDuration,
	const Entity* station)
{
	auto shooter{ std::make_unique<Saucer>(assets, *this, Saucer::Mode::Shooter) };
	shooter->SetPosition(position);
	shooter->SetRewardsEnabled(false);
	shooter->BeginMaterialization(materializationDuration, station);
	Spawn(std::move(shooter));
}

void World::DamageEnemiesWithPlayerLaser(
	const sf::Vector2f& start,
	const sf::Vector2f& end,
	float laserWidth,
	int damage,
	std::uint64_t attackID)
{
	const sf::Vector2f segment{ end - start };
	if (segment.x * segment.x + segment.y * segment.y <= 0.001f || damage <= 0)
		return;
	playerLaserDamageEvent = PlayerLaserDamageEvent{
		start, end, laserWidth, damage, attackID };

	for (const auto& entity : entities)
	{
		if (!entity->IsAlive())
			continue;
		const Entity::Type type{ entity->GetType() };
		if (type != Entity::Type::Enemy && type != Entity::Type::Asteroid &&
			type != Entity::Type::EnemyMissile)
		{
			continue;
		}

		const auto impactPosition{ ProjectileGeometry::TrySegmentImpactPoint(
			start, end, laserWidth, entity->GetPosition(), entity->GetCollisionRadius()) };
		if (!impactPosition)
			continue;

		RegisterPlayerAttackHit(attackID);
		effectEvents.Add({ Rendering::EffectEventType::ShipHit,
			*impactPosition, Normalize(segment),
			std::clamp(entity->GetCollisionRadius() / 45.f, 0.55f, 1.2f) });
		if (type == Entity::Type::EnemyMissile)
		{
			static_cast<void>(
				static_cast<HomingMissile&>(*entity).TakeDamage(damage));
			continue;
		}

		Enemy& enemy{ static_cast<Enemy&>(*entity) };
		if (enemy.BlocksPlayerLaser())
			continue;
		if (enemy.TakeDamage(damage))
			AwardScore(enemy);
	}
}

void World::DamagePlayerWithBeam(
	const sf::Vector2f& start,
	const sf::Vector2f& end,
	float beamWidth,
	int damage)
{
	if (player == nullptr || !player->IsAlive())
		return;
	const sf::Vector2f segment{ end - start };
	const auto closest{ ProjectileGeometry::TrySegmentImpactPoint(
		start, end, beamWidth, player->GetPosition(), player->GetCollisionRadius()) };
	if (!closest)
		return;

	if (player->TakeDamage(damage))
	{
		if (player->DidLastDamageReachHealth())
			effectEvents.Add({ Rendering::EffectEventType::PlayerHit,
				*closest, Normalize(segment), 1.15f });
		sound.AddSound(Config::Sound::MetalHit, 0.85f);
		if (!player->IsAlive())
			session.SetGameOver();
	}
}

void World::ExplodeEnemyMissile(
	const sf::Vector2f& position,
	float radius,
	int damage,
	float impulse)
{
	for (auto& entityPointer : entities)
	{
		Entity& entity{ *entityPointer };
		if (!entity.IsAlive())
			continue;

		const sf::Vector2f offset{ entity.GetPosition() - position };
		const float reach{ radius + entity.GetCollisionRadius() };
		if (offset.x * offset.x + offset.y * offset.y > reach * reach)
			continue;
		const sf::Vector2f direction{ Normalize(offset) };

		if (entity.GetType() == Entity::Type::EnemyMissile)
		{
			static_cast<HomingMissile&>(entity).Detonate();
			continue;
		}
		if (entity.GetType() == Entity::Type::Player)
		{
			auto& targetPlayer{ static_cast<Player&>(entity) };
			const bool damageAccepted{ targetPlayer.TakeDamage(damage) };
			if (damageAccepted)
			{
				if (targetPlayer.DidLastDamageReachHealth())
					effectEvents.Add({ Rendering::EffectEventType::PlayerHit,
						targetPlayer.GetPosition(), direction, 1.25f });
				targetPlayer.ApplyImpulse(direction * impulse);
			}
			if (!targetPlayer.IsAlive())
				session.SetGameOver();
			continue;
		}
		if (entity.GetType() != Entity::Type::Enemy &&
			entity.GetType() != Entity::Type::Asteroid)
		{
			continue;
		}

		auto& enemy{ static_cast<Enemy&>(entity) };
		effectEvents.Add({
			entity.GetType() == Entity::Type::Asteroid
				? Rendering::EffectEventType::AsteroidHit
				: Rendering::EffectEventType::ShipHit,
			enemy.GetPosition(), direction,
			std::clamp(enemy.GetCollisionRadius() / 45.f, 0.65f, 1.25f) });
		const bool killed{ enemy.TakeDamage(damage) };
			if (enemy.AcceptsKnockback())
				enemy.ApplyImpulse(direction * impulse);
		if (killed)
			AwardScore(enemy);
	}
}

WorldSoundSystem& World::Sound() noexcept { return sound; }
WorldEffectEventQueue& World::Effects() noexcept { return effectEvents; }

void World::CompleteDelayedEnemyDestruction(Enemy& enemy)
{
	AwardScore(enemy);
}

void World::ClearProjectiles()
{
	const auto isProjectile{ [](const auto& entity)
	{
		return entity->GetType() == Entity::Type::Projectile_Player ||
			entity->GetType() == Entity::Type::Projectile_Ally ||
			entity->GetType() == Entity::Type::Projectile_Enemy ||
			entity->GetType() == Entity::Type::EnemyMissile;
	} };
	std::erase_if(entities, isProjectile);
	std::erase_if(pendingEntities, isProjectile);
}

std::optional<World::PlayerLaserDamageEvent>
World::ConsumePlayerLaserDamageEvent() noexcept
{
	return std::exchange(playerLaserDamageEvent, std::nullopt);
}

std::vector<World::PlayerProjectileImpact> World::ConsumePlayerProjectilesInCircle(
	sf::Vector2f center, float radius)
{
	std::vector<PlayerProjectileImpact> impacts;
	for (const auto& entity : entities)
	{
		if (!entity->IsAlive() ||
			entity->GetType() != Entity::Type::Projectile_Player)
		{
			continue;
		}

		const auto direction{ ProjectileGeometry::TryCircleImpactDirection(
			center, radius, entity->GetPosition(), entity->GetCollisionRadius()) };
		if (!direction)
			continue;

		const Shot& shot{ static_cast<const Shot&>(*entity) };
		impacts.push_back({ center + *direction * radius, shot.GetDamage() });
		RegisterPlayerAttackHit(shot.GetPlayerAttackID());
		entity->Destroy();
		effectEvents.Add({ Rendering::EffectEventType::ShipHit,
			impacts.back().position, *direction, 1.25f });
		sound.AddSound(Config::Sound::MetalHit, 0.72f);
	}
	return impacts;
}

std::vector<World::PlayerProjectileImpact> World::ConsumePlayerProjectilesInAnnulus(
	sf::Vector2f center, float innerRadius, float outerRadius)
{
	std::vector<PlayerProjectileImpact> impacts;
	for (const auto& entity : entities)
	{
		if (!entity->IsAlive() ||
			entity->GetType() != Entity::Type::Projectile_Player)
		{
			continue;
		}

		const auto direction{ ProjectileGeometry::TryAnnulusImpactDirection(
			center, innerRadius, outerRadius,
			entity->GetPosition(), entity->GetCollisionRadius()) };
		if (!direction)
			continue;

		const Shot& shot{ static_cast<const Shot&>(*entity) };
		impacts.push_back({ center + *direction * outerRadius, shot.GetDamage() });
		RegisterPlayerAttackHit(shot.GetPlayerAttackID());
		entity->Destroy();
		effectEvents.Add({ Rendering::EffectEventType::ShipHit,
			impacts.back().position, *direction, 1.15f });
		sound.AddSound(Config::Sound::MetalHit, 0.82f);
	}
	return impacts;
}

void World::ConsumePlayerProjectilesInDiamondFrame(
	sf::Vector2f center,
	float rotationDegrees,
	float vertexRadius,
	float halfThickness)
{
	for (const auto& entity : entities)
	{
		if (!entity->IsAlive() ||
			entity->GetType() != Entity::Type::Projectile_Player)
		{
			continue;
		}

		const auto direction{ ProjectileGeometry::TryDiamondFrameImpactDirection(
			center, rotationDegrees, vertexRadius, halfThickness,
			entity->GetPosition(), entity->GetCollisionRadius()) };
		if (!direction)
			continue;

		const Shot& shot{ static_cast<const Shot&>(*entity) };
		RegisterPlayerAttackHit(shot.GetPlayerAttackID());
		entity->Destroy();
		effectEvents.Add({ Rendering::EffectEventType::ShipHit,
			entity->GetPosition(), *direction, 0.9f });
		sound.AddSound(Config::Sound::MetalHit, 0.76f);
	}
}

bool World::DamagePlayerFromBoss(int damage, sf::Vector2f sourcePosition)
{
	if (player == nullptr || !player->IsAlive() || damage <= 0)
		return false;
	if (!player->TakeDamage(damage))
		return false;

	const sf::Vector2f direction{ Normalize(player->GetPosition() - sourcePosition) };
	if (player->DidLastDamageReachHealth())
	{
		effectEvents.Add({ Rendering::EffectEventType::PlayerHit,
			player->GetPosition(), direction, 1.2f });
	}
	sound.AddSound(Config::Sound::MetalHit, 1.08f);
	if (!player->IsAlive())
		session.SetGameOver();
	return true;
}

void World::KeepPlayerOutsideCircle(
	sf::Vector2f center,
	float radius,
	int contactDamage)
{
	if (player == nullptr || !player->IsAlive())
		return;
	const float minimumDistance{ radius + player->GetCollisionRadius() };
	const sf::Vector2f offset{ player->GetPosition() - center };
	const float distanceSquared{ offset.x * offset.x + offset.y * offset.y };
	if (distanceSquared >= minimumDistance * minimumDistance)
		return;

	const float distance{ std::sqrt(distanceSquared) };
	const sf::Vector2f direction{ distance > 0.001f
		? offset / distance
		: sf::Vector2f{ 0.f, 1.f } };
	player->SetPosition(center + direction * minimumDistance);
	const sf::Vector2f velocity{ player->GetVelocity() };
	const float inwardSpeed{ velocity.x * direction.x + velocity.y * direction.y };
	if (inwardSpeed < 0.f)
		player->SetVelocity(velocity - direction * inwardSpeed * 1.25f);
	static_cast<void>(DamagePlayerFromBoss(contactDamage, center));
}

void World::KeepEnemiesOutsideCircle(
	sf::Vector2f center,
	float radius,
	float clearance)
{
	const auto keepOutside{ [&](auto& list)
	{
		for (auto& entity : list)
		{
			if (!entity->IsAlive() || entity->GetType() != Entity::Type::Enemy)
				continue;

			const float minimumDistance{
				radius + clearance + entity->GetCollisionRadius() };
			const sf::Vector2f offset{ entity->GetPosition() - center };
			const float distanceSquared{ offset.x * offset.x + offset.y * offset.y };
			if (distanceSquared >= minimumDistance * minimumDistance)
				continue;

			const float distance{ std::sqrt(distanceSquared) };
			const sf::Vector2f direction{ distance > 0.001f
				? offset / distance
				: sf::Vector2f{ 0.f, 1.f } };
			const sf::Vector2f velocity{ entity->GetVelocity() };
			const float inwardSpeed{
				velocity.x * direction.x + velocity.y * direction.y };
			if (inwardSpeed > 0.f)
				continue;
			entity->SetPosition(center + direction * minimumDistance);
			if (inwardSpeed < 0.f)
				entity->SetVelocity(velocity - direction * inwardSpeed);
		}
	} };
	keepOutside(entities);
	keepOutside(pendingEntities);
}

void World::SetRewardExclusionCircle(sf::Vector2f center, float radius) noexcept
{
	rewardExclusionZone.Set(center, radius);
}

std::size_t World::CountActiveLaserTurrets() const noexcept
{
	const auto countIn{ [](const auto& list)
	{
		return static_cast<std::size_t>(std::count_if(
			list.begin(), list.end(), [](const auto& entity)
			{
				return entity->IsAlive() &&
					dynamic_cast<const LaserTurret*>(entity.get()) != nullptr;
			}));
	} };
	return countIn(entities) + countIn(pendingEntities);
}

bool World::DestroyNextBossVictoryTarget()
{
	const auto destroyNextIn{ [this](auto& list)
	{
		for (const auto& entity : list)
		{
			if (!entity->IsAlive())
				continue;
			const Entity::Type type{ entity->GetType() };
			if (type != Entity::Type::Enemy && type != Entity::Type::Asteroid &&
				type != Entity::Type::EnemyMissile)
			{
				continue;
			}
			if (type == Entity::Type::Enemy || type == Entity::Type::Asteroid)
			{
				Enemy& enemy{ static_cast<Enemy&>(*entity) };
				enemy.SetScoreRewardEnabled(false);
				enemy.SetPickupRewardsEnabled(false);
				enemy.SetPartRewardEnabled(false);
				if (auto* meteor{ dynamic_cast<Meteor*>(entity.get()) })
					meteor->SetFragmentSpawningEnabled(false);
			}
			else
			{
				effectEvents.Add({ Rendering::EffectEventType::ShipExplosion,
					entity->GetPosition(), {}, 0.7f });
			}
			entity->Destroy();
			return true;
		}
		return false;
	} };
	return destroyNextIn(entities) || destroyNextIn(pendingEntities);
}

void World::DestroyAllBossVictoryTargets()
{
	while (DestroyNextBossVictoryTarget())
	{
	}
}

void World::ClearBossVictoryPickupsAndCompanions()
{
	const auto remove{ [](const auto& entity)
	{
		const Entity::Type type{ entity->GetType() };
		return type == Entity::Type::Pickup || type == Entity::Type::Part ||
			type == Entity::Type::Companion;
	} };
	std::erase_if(entities, remove);
	std::erase_if(pendingEntities, remove);
}

void World::ClearPickups()
{
	const auto isPickup{ [](const auto& entity)
	{
		return entity->GetType() == Entity::Type::Pickup ||
			entity->GetType() == Entity::Type::Part;
	} };
	std::erase_if(entities, isPickup);
	std::erase_if(pendingEntities, isPickup);
}

void World::ConfigureCampaignPickupSequence(
	const std::vector<GameplayData::PickupKind>& sequence)
{
	campaignPickupQueue.Configure(sequence);
}

sf::Vector2f World::GetPlayerPosition() const noexcept
{
	return player != nullptr ? player->GetPosition() : sf::Vector2f{};
}

const Entity* World::FindHomingTarget(
	const sf::Vector2f& position,
	const sf::Vector2f& direction,
	float minimumDirectionDot) const noexcept
{
	const sf::Vector2f normalizedDirection{ Normalize(direction) };
	const Entity* closestTarget{ nullptr };
	float closestDistanceSquared{ std::numeric_limits<float>::max() };

	for (const auto& entity : entities)
	{
		if (!entity->IsAlive() ||
			(entity->GetType() != Entity::Type::Enemy &&
			 entity->GetType() != Entity::Type::Asteroid &&
			 entity->GetType() != Entity::Type::EnemyMissile))
		{
			continue;
		}

		const sf::Vector2f offset{ entity->GetPosition() - position };
		const float distanceSquared{ offset.x * offset.x + offset.y * offset.y };
		if (distanceSquared <= 0.0001f || distanceSquared >= closestDistanceSquared)
			continue;

		const float inverseDistance{ 1.f / std::sqrt(distanceSquared) };
		const float directionDot{
			(offset.x * normalizedDirection.x + offset.y * normalizedDirection.y) *
			inverseDistance };
		if (directionDot < minimumDirectionDot)
			continue;

		closestTarget = entity.get();
		closestDistanceSquared = distanceSquared;
	}

	return closestTarget;
}

WorldBossHomingTargets& World::BossHomingTargets() noexcept { return bossHomingTargets; }

bool World::IsEntityActive(const Entity* entity) const noexcept
{
	return entity != nullptr && std::ranges::any_of(entities,
		[entity](const auto& candidate)
		{
			return candidate.get() == entity && candidate->IsAlive();
		});
}

unsigned int World::GetWidth() const noexcept { return width; }
unsigned int World::GetHeight() const noexcept { return height; }
GameplaySession& World::GetSession() noexcept { return session; }
const World::Statistics& World::GetStatistics() const noexcept { return statisticsTracker.Get(); }

void World::Clear()
{
	entities.clear();
	pendingEntities.clear();
	rewardExclusionZone.Clear();
	bossHomingTargets.Clear();
	playerLaserDamageEvent.reset();
	effectEvents.Clear();
	player = nullptr;
	shieldVisualTime = 0.f;
	statisticsTracker.Reset();
	playerAttackTracker.Reset();
	helperPickupSpawned = false;
	helperBotSpawned = false;
	campaignPickupQueue.Clear();
}

void World::Wrap(Entity& entity) const
{
	auto position{ entity.GetPosition() };
	if (position.x < 0.f) position.x = static_cast<float>(width);
	else if (position.x > width) position.x = 0.f;
	if (position.y < 0.f) position.y = static_cast<float>(height);
	else if (position.y > height) position.y = 0.f;
	entity.SetPosition(position);
}

void World::HandleCollisions()
{
	// Broad-phase: bucket entities into a uniform grid so only nearby pairs
	// are narrow-phase tested, instead of every pair in the world (O(n^2)).
	// The cell size must stay >= the largest possible sum of two entities'
	// collision radii (biggest enemy is ~114 px, so ~230 px combined); with a
	// 256 px cell, any two entities close enough to touch are guaranteed to
	// land in the same cell or an immediately adjacent one, so scanning the
	// 3x3 neighborhood around an entity's cell can't miss a real collision.
	// The grid wraps toroidally to match World::Wrap's edge behavior.
	constexpr float CellSize{ 256.f };
	const int columns{ std::max(1, static_cast<int>(std::ceil(static_cast<float>(width) / CellSize))) };
	const int rows{ std::max(1, static_cast<int>(std::ceil(static_cast<float>(height) / CellSize))) };

	const auto cellIndex{ [columns, rows](int cx, int cy) -> std::size_t
		{
			const int wrappedX{ ((cx % columns) + columns) % columns };
			const int wrappedY{ ((cy % rows) + rows) % rows };
			return static_cast<std::size_t>(wrappedY) * static_cast<std::size_t>(columns) +
				static_cast<std::size_t>(wrappedX);
		} };

	std::vector<std::vector<std::size_t>> grid(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
	std::vector<std::pair<int, int>> entityCell(entities.size());
	for (std::size_t index{ 0u }; index < entities.size(); ++index)
	{
		if (!entities[index]->IsAlive())
			continue;
		const sf::Vector2f position{ entities[index]->GetPosition() };
		const int cx{ static_cast<int>(std::floor(position.x / CellSize)) };
		const int cy{ static_cast<int>(std::floor(position.y / CellSize)) };
		entityCell[index] = { cx, cy };
		grid[cellIndex(cx, cy)].push_back(index);
	}

	for (std::size_t i{ 0u }; i < entities.size(); ++i)
	{
		if (!entities[i]->IsAlive())
			continue;
		const auto [cx, cy]{ entityCell[i] };
		for (int dy{ -1 }; dy <= 1; ++dy)
		{
			for (int dx{ -1 }; dx <= 1; ++dx)
			{
				for (const std::size_t j : grid[cellIndex(cx + dx, cy + dy)])
				{
					if (j <= i)
						continue;

					Entity& first{ *entities[i] };
					Entity& second{ *entities[j] };
					// Re-checked per pair (not hoisted) because handling an
					// earlier pair in this same neighborhood scan may have
					// destroyed `first` or `second` already.
					if (first.IsAlive() && second.IsAlive() &&
						first.IsCollideWith(second) && second.IsCollideWith(first))
					{
						HandleCollisionPair(first, second);
					}
				}
			}
		}
	}
}

void World::HandleCollisionPair(Entity& first, Entity& second)
{
	if (first.GetType() == Entity::Type::Part || second.GetType() == Entity::Type::Part)
	{
		Part& part{ static_cast<Part&>(
			first.GetType() == Entity::Type::Part ? first : second) };
		Entity& other{ first.GetType() == Entity::Type::Part ? second : first };
		if (other.GetType() == Entity::Type::Player)
		{
			if (session.RecoverPart(part.GetID()))
				sound.AddSound(Config::Sound::PartPickedUp);
			part.Destroy();
		}
		return;
	}

	if (first.GetType() == Entity::Type::Pickup || second.GetType() == Entity::Type::Pickup)
	{
		Pickup& pickup{ static_cast<Pickup&>(
			first.GetType() == Entity::Type::Pickup ? first : second) };
		Entity& other{ first.GetType() == Entity::Type::Pickup ? second : first };
		if (other.GetType() == Entity::Type::Player && pickup.Apply(session))
		{
			sound.AddSound(Config::Sound::BonusTouched);
			if (pickup.GetKind() == Pickup::Kind::Shield)
				statisticsTracker.RecordShieldPickupCollected();
			else if (pickup.GetKind() == Pickup::Kind::HelperBot)
				SpawnHelperBot();
			pickup.Destroy();
		}
		return;
	}

	if (first.GetType() == Entity::Type::EnemyMissile ||
		second.GetType() == Entity::Type::EnemyMissile)
	{
		auto& missile{ static_cast<HomingMissile&>(
			first.GetType() == Entity::Type::EnemyMissile ? first : second) };
		Entity& other{ first.GetType() == Entity::Type::EnemyMissile ? second : first };
		if (other.GetType() == Entity::Type::Projectile_Player ||
			other.GetType() == Entity::Type::Projectile_Ally)
		{
			auto& shot{ static_cast<Shot&>(other) };
			RegisterPlayerAttackHit(shot.GetPlayerAttackID());
			effectEvents.Add({ Rendering::EffectEventType::ShipHit,
				missile.GetPosition(), Normalize(shot.GetVelocity()), 0.55f });
			shot.Destroy();
			if (!missile.TakeDamage(shot.GetDamage()))
				sound.AddSound(Config::Sound::MetalHit, 1.2f);
		}
		else
		{
			missile.Detonate();
			if (other.GetType() == Entity::Type::EnemyMissile)
				static_cast<HomingMissile&>(other).Detonate();
		}
		return;
	}

	auto handleFriendlyShot = [this](Shot& shot, Enemy& enemy)
	{
		RegisterPlayerAttackHit(shot.GetPlayerAttackID());
		const sf::Vector2f impactDirection{ Normalize(shot.GetVelocity()) };
		const sf::Vector2f impactPosition{
			enemy.GetPlayerProjectileImpactPosition(shot) };
		effectEvents.Add({
			enemy.GetType() == Entity::Type::Asteroid
				? Rendering::EffectEventType::AsteroidHit
				: Rendering::EffectEventType::ShipHit,
			impactPosition,
			impactDirection,
			std::clamp(enemy.GetCollisionRadius() / 45.f, 0.55f, 1.15f) });
		if (enemy.IsProjectileReflectionActive())
		{
			const sf::Vector2f reflectedDirection{
				Normalize(GetPlayerPosition() - impactPosition) };
			shot.ReflectToward(GetPlayerPosition());
			shot.Translate(reflectedDirection * 12.f);
			sound.AddSound(Config::Sound::EnemyShot, 0.72f);
			return;
		}
		shot.Destroy();
		const bool killed{ enemy.TakeDamage(shot.GetDamage()) };
		if (enemy.AcceptsKnockback())
			enemy.ApplyImpulse(impactDirection * shot.GetKnockback());
		if (killed)
			AwardScore(enemy);
		else if (enemy.GetType() == Entity::Type::Asteroid)
			sound.AddSound(Config::Sound::BulletHitAsteroid, enemy.GetSoundPitch());
		else if (enemy.GetType() == Entity::Type::Enemy)
			sound.AddSound(Config::Sound::MetalHit);
	};

	if (first.GetType() == Entity::Type::Projectile_Player)
	{
		handleFriendlyShot(static_cast<Shot&>(first), static_cast<Enemy&>(second));
		return;
	}
	if (first.GetType() == Entity::Type::Projectile_Ally)
	{
		handleFriendlyShot(static_cast<Shot&>(first), static_cast<Enemy&>(second));
		return;
	}
	if (second.GetType() == Entity::Type::Projectile_Player ||
		second.GetType() == Entity::Type::Projectile_Ally)
	{
		handleFriendlyShot(static_cast<Shot&>(second), static_cast<Enemy&>(first));
		return;
	}

	auto handleEnemyShot = [this](Shot& shot, Entity& target)
	{
		shot.Destroy();
		if (target.GetType() == Entity::Type::Player)
		{
			auto& targetPlayer{ static_cast<Player&>(target) };
			const bool damageAccepted{ targetPlayer.TakeDamage(shot.GetDamage()) };
			if (damageAccepted)
			{
				if (targetPlayer.DidLastDamageReachHealth())
					effectEvents.Add({ Rendering::EffectEventType::PlayerHit,
						shot.GetPosition(), Normalize(shot.GetVelocity()), 1.f });
				targetPlayer.ApplyImpulse(Normalize(shot.GetVelocity()) * shot.GetKnockback());
			}
			if (damageAccepted && targetPlayer.IsAlive())
				sound.AddSound(Config::Sound::MetalHit);
			if (!targetPlayer.IsAlive())
				session.SetGameOver();
		}
		else
		{
			auto& enemy{ static_cast<Enemy&>(target) };
			const bool destroyed{ enemy.TakeDamage(shot.GetDamage()) };
			(void)destroyed;
			if (enemy.AcceptsKnockback())
				enemy.ApplyImpulse(Normalize(shot.GetVelocity()) * shot.GetKnockback());
		}
	};

	if (first.GetType() == Entity::Type::Projectile_Enemy)
	{
		handleEnemyShot(static_cast<Shot&>(first), second);
		return;
	}
	if (second.GetType() == Entity::Type::Projectile_Enemy)
	{
		handleEnemyShot(static_cast<Shot&>(second), first);
		return;
	}

	Player* collidedPlayer{ nullptr };
	Enemy* collidedEnemy{ nullptr };
	if (first.GetType() == Entity::Type::Player)
	{
		collidedPlayer = static_cast<Player*>(&first);
		collidedEnemy = static_cast<Enemy*>(&second);
	}
	else if (second.GetType() == Entity::Type::Player)
	{
		collidedPlayer = static_cast<Player*>(&second);
		collidedEnemy = static_cast<Enemy*>(&first);
	}
	else
	{
		return;
	}

	const bool damageAccepted{ collidedPlayer->TakeDamage(collidedEnemy->GetContactDamage()) };
	if (damageAccepted)
	{
		const sf::Vector2f impactDirection{ Normalize(
			collidedEnemy->GetPosition() - collidedPlayer->GetPosition()) };
		const sf::Vector2f impactPosition{ collidedPlayer->GetPosition() +
			impactDirection * collidedPlayer->GetCollisionRadius() };
		if (collidedPlayer->DidLastDamageReachHealth())
			effectEvents.Add({ Rendering::EffectEventType::PlayerHit,
				impactPosition, -impactDirection, 1.15f });
		effectEvents.Add({
			collidedEnemy->GetType() == Entity::Type::Asteroid
				? Rendering::EffectEventType::AsteroidHit
				: Rendering::EffectEventType::ShipHit,
			impactPosition,
			impactDirection,
			1.1f });

		const Config::Sound impactSound{ collidedEnemy->GetType() == Entity::Type::Asteroid
			? Config::Sound::HitAsteroid
			: Config::Sound::HitEnemySaucer };
		sound.AddSound(impactSound, collidedEnemy->GetSoundPitch());

		const bool enemyKilled{ collidedEnemy->TakeDamage(
			assets.GetGameplayData().GetPlayer().collisionDamage) };
		if (enemyKilled)
			AwardScore(*collidedEnemy);
	}

	if (!collidedPlayer->IsAlive())
		session.SetGameOver();

	if (!collidedPlayer->IsAlive() || !collidedEnemy->IsAlive())
		return;

	const auto manifold{ collidedPlayer->GetCollisionContactInfo(*collidedEnemy) };
	if (!manifold)
		return;

	ResolveCollision(*collidedPlayer, *collidedEnemy,
		manifold->normal, manifold->penetration);
	if (damageAccepted)
	{
		collidedPlayer->ApplyImpulse(-manifold->normal * collidedEnemy->GetCollisionImpulse());
		if (collidedEnemy->AcceptsKnockback())
			collidedEnemy->ApplyImpulse(manifold->normal *
				assets.GetGameplayData().GetPlayer().collisionImpulse);
	}
}

void World::AwardScore(const Enemy& enemy)
{
	if (!enemy.AreRewardsEnabled())
		return;
	if (enemy.IsScoreRewardEnabled())
	{
		if (const auto* meteor{ dynamic_cast<const Meteor*>(&enemy) })
		{
			if (meteor->GetSize() == Meteor::Size::Big)
				statisticsTracker.RecordBigMeteorDestroyed();
			else
				statisticsTracker.RecordSmallMeteorDestroyed();
		}
		else if (const auto* saucer{ dynamic_cast<const Saucer*>(&enemy) };
			saucer != nullptr && saucer->GetMode() == Saucer::Mode::Shooter)
		{
			statisticsTracker.RecordShooterDestroyed();
		}

		const int points{ enemy.GetScoreValue() };
		session.AddScore(points);
		effectEvents.Add({
			Rendering::EffectEventType::ScorePopup,
			enemy.GetPosition(),
			{},
			1.f,
			points });
	}

	std::vector<GameplayData::PickupKind> pickupKinds;
	if (enemy.ArePickupRewardsEnabled())
	{
		const int orderedDropCount{ enemy.GetOrderedPickupDropCount() };
		if (orderedDropCount > 0)
		{
			for (int index{ 0 }; index < orderedDropCount; ++index)
			{
				const auto pickupKind{ campaignPickupQueue.TryPop() };
				if (!pickupKind)
					break;
				pickupKinds.push_back(*pickupKind);
			}
		}
		else if (const auto pickupKind{ enemy.RollPickupDrop() })
		{
			pickupKinds.push_back(*pickupKind);
		}
	}
	for (std::size_t index{ 0u }; index < pickupKinds.size(); ++index)
	{
		const float angle{ pickupKinds.size() > 1u
			? 2.f * std::numbers::pi_v<float> *
				static_cast<float>(index) / static_cast<float>(pickupKinds.size())
			: 0.f };
		const float radius{ pickupKinds.size() > 1u ? 72.f : 0.f };
		const sf::Vector2f position{ rewardExclusionZone.PushOutside(
			enemy.GetPosition() + sf::Vector2f{
				std::cos(angle) * radius, std::sin(angle) * radius }) };
		if (pickupKinds[index] == Pickup::Kind::HelperBot)
			static_cast<void>(SpawnHelperPickup(position));
		else
		{
			auto pickup{ std::make_unique<Pickup>(
				assets, *this, pickupKinds[index]) };
			pickup->SetPosition(position);
			Spawn(std::move(pickup));
		}
	}

	if (enemy.IsPartRewardEnabled() && !enemy.GetPartDropID().empty() &&
		!session.IsPartCollected(enemy.GetPartDropID()))
	{
		auto part{ std::make_unique<Part>(assets, *this, enemy.GetPartDropID()) };
		part->SetPosition(enemy.GetPosition());
		Spawn(std::move(part));
	}
}

void World::ResolveCollision(Entity& first, Entity& second,
	const sf::Vector2f& normal, float penetration) const
{
	const sf::Vector2f correction{ normal * ((penetration + 0.5f) * 0.5f) };
	first.Translate(-correction);
	second.Translate(correction);
}

void World::RemoveDeadEntities()
{
	for (std::size_t i{ 0 }; i < entities.size();)
	{
		if (!entities[i]->IsAlive())
		{
			if (entities[i].get() == player)
				player = nullptr;
			entities[i] = std::move(entities.back());
			entities.pop_back();
		}
		else
		{
			++i;
		}
	}
}

void World::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	sf::Shader& enemyEmissionShader{
		assets.GetShader(Config::Shader::EnemyEmission) };
	enemyEmissionShader.setUniform("source", sf::Shader::CurrentTexture);
	sf::Shader& playerEmissionShader{
		assets.GetShader(Config::Shader::PlayerEmission) };
	playerEmissionShader.setUniform("source", sf::Shader::CurrentTexture);
	const auto drawEntity = [&](const std::unique_ptr<Entity>& entity)
	{
		if (const auto* turret{ dynamic_cast<const LaserTurret*>(entity.get()) })
			DrawLaserBeam(target, *turret, states);
		if (entity->GetType() == Entity::Type::Pickup)
			DrawPickupAura(target, static_cast<const Pickup&>(*entity), shieldVisualTime, states);
		else if (entity->GetType() == Entity::Type::Part)
			DrawPartAura(target, static_cast<const Part&>(*entity), shieldVisualTime, states);

		target.draw(*entity, states);
		if (const auto* station{ dynamic_cast<const ShooterStation*>(entity.get()) })
			DrawStationEffects(target, *station, states);
		if (const auto* reflector{ dynamic_cast<const ReflectorGunship*>(entity.get()) };
			reflector != nullptr && reflector->IsShieldActive())
		{
			Rendering::DrawEnergyShield(target, reflector->GetPosition(),
				reflector->GetShieldRadius(), reflector->GetShieldPulse(),
				{ 205, 20, 35 }, { 255, 65, 75 }, { 255, 35, 45 }, 58.f, states);
		}
		if (entity->GetType() == Entity::Type::Enemy ||
			entity->GetType() == Entity::Type::EnemyMissile)
		{
			sf::RenderStates emissionStates{ states };
			emissionStates.shader = &enemyEmissionShader;
			emissionStates.blendMode = sf::BlendAdd;
			target.draw(entity->GetSprite(), emissionStates);
		}
		else if (entity->GetType() == Entity::Type::Player ||
			entity->GetType() == Entity::Type::Companion)
		{
			sf::RenderStates emissionStates{ states };
			emissionStates.shader = &playerEmissionShader;
			emissionStates.blendMode = sf::BlendAdd;
			target.draw(entity->GetSprite(), emissionStates);
		}

		const Shield& shield{ session.GetPlayerShield() };
		if (entity.get() == player && (shield.IsActive() || shield.IsHitFlashing()))
		{
			DrawShieldOverlay(
				target,
				*player,
				shieldVisualTime,
				shield.GetRatio(),
				shield.GetHitFlashRatio(),
				states);
		}
	};

	// Stations form the background layer of their encounters. Drawing them in a
	// dedicated pass guarantees that materialized shooters remain visible above
	// the launch bay even when entity removal changes the vector order.
	for (const auto& entity : entities)
	{
		if (dynamic_cast<const ShooterStation*>(entity.get()))
			drawEntity(entity);
	}
	if (player != nullptr)
	{
		DrawPlayerLaser(target, *player,
			assets.GetGameplayData().GetPickups().laserWidth, states);
	}
	for (const auto& entity : entities)
	{
		if (!dynamic_cast<const ShooterStation*>(entity.get()))
			drawEntity(entity);
	}
}
