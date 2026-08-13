#include "World.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "entities/Enemy.h"
#include "entities/HomingMissile.h"
#include "entities/Meteor.h"
#include "entities/Player.h"
#include "entities/Pickup.h"
#include "entities/Saucer.h"
#include "entities/Shot.h"
#include "game/GameplaySession.h"
#include "systems/Collision.h"
#include "systems/GamepadManager.h"

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

		const float radius{ player.GetCollisionRadius() * 1.62f };
		const sf::Vector2f center{ player.GetPosition() };

		sf::CircleShape shell(radius, 96u);
		shell.setOrigin({ radius, radius });
		shell.setPosition(center);
		shell.setFillColor(sf::Color(20, 205, 235,
			static_cast<std::uint8_t>(27.f * pulse)));
		shell.setOutlineColor(sf::Color(70, 240, 255,
			static_cast<std::uint8_t>(58.f * pulse)));
		shell.setOutlineThickness(4.f);
		target.draw(shell, states);

		sf::RenderStates additiveStates{ states };
		additiveStates.blendMode = sf::BlendAdd;
		for (int layer{ 0 }; layer < 3; ++layer)
		{
			const float glowRadius{ radius + 2.f + layer * 3.f };
			sf::CircleShape glow(glowRadius, 96u);
			glow.setOrigin({ glowRadius, glowRadius });
			glow.setPosition(center);
			glow.setFillColor(sf::Color::Transparent);
			glow.setOutlineColor(sf::Color(45, 225, 255,
				static_cast<std::uint8_t>((34.f - layer * 8.f) * pulse)));
			glow.setOutlineThickness(7.f + layer * 3.f);
			target.draw(glow, additiveStates);
		}

		constexpr float HexRadius{ 9.f };
		constexpr float HorizontalSpacing{ 16.f };
		constexpr float VerticalSpacing{ 14.f };
		for (int row{ -3 }; row <= 3; ++row)
		{
			for (int column{ -4 }; column <= 4; ++column)
			{
				const float x{ column * HorizontalSpacing + (row % 2 == 0 ? 0.f : 8.f) };
				const float y{ row * VerticalSpacing };
				if (x * x + y * y > (radius - HexRadius - 3.f) * (radius - HexRadius - 3.f))
					continue;

				sf::CircleShape hex(HexRadius, 6u);
				hex.setOrigin({ HexRadius, HexRadius });
				hex.setPosition(center + sf::Vector2f{ x, y });
				hex.setRotation(sf::degrees(30.f));
				hex.setFillColor(sf::Color::Transparent);
				hex.setOutlineColor(sf::Color(70, 235, 255,
					static_cast<std::uint8_t>(24.f * pulse)));
				hex.setOutlineThickness(1.f);
				target.draw(hex, additiveStates);
			}
		}
	}
}

World::World(unsigned int width, unsigned int height, AssetStore& assets,
	AudioManager& audio, GameplaySession& session, GamepadManager& gamepadManager)
	: assets(assets), audio(audio), session(session), gamepad(gamepadManager),
	  width(width), height(height)
{
	effectEvents.reserve(256);
}

void World::Update(float deltaTime, float worldTimeScale)
{
	shieldVisualTime += deltaTime;
	worldTimeScale = std::clamp(worldTimeScale, 0.f, 1.f);
	CommitPendingEntities();

	for (auto& entity : entities)
	{
		const Entity::Type type{ entity->GetType() };
		const bool usesPlayerTime{
			type == Entity::Type::Player ||
			type == Entity::Type::Projectile_Player ||
			type == Entity::Type::Pickup };
		const float entityDeltaTime{ usesPlayerTime
			? deltaTime
			: deltaTime * worldTimeScale };
		entity->UpdateEffects(entityDeltaTime);
		entity->Update(entityDeltaTime);
		if (entity->GetType() != Entity::Type::Projectile_Player &&
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

void World::SpawnPlayer(AssetStore& playerAssets, InputHandler<Config::PlayerAction>& input)
{
	if (player != nullptr)
		return;

	auto playerPtr{ std::make_unique<Player>(playerAssets, *this, input, gamepad) };
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

sf::RenderWindow& World::GetWindow() noexcept { return *window; }
void World::SetWindow(sf::RenderWindow& newWindow) { window = &newWindow; }

bool World::IsCleared() const noexcept
{
	const auto containsAliveEnemy = [](const auto& list)
	{
		return std::ranges::any_of(list, [](const auto& entity)
		{
			return entity->IsAlive() &&
				(entity->GetType() == Entity::Type::Enemy ||
				 entity->GetType() == Entity::Type::Asteroid);
		});
	};

	return !containsAliveEnemy(entities) && !containsAliveEnemy(pendingEntities);
}

bool World::HasPlayer() const noexcept { return player != nullptr; }

void World::HandlePlayerEvent(const sf::Event& event)
{
	if (player != nullptr)
		player->HandleEvent(event);
}

void World::HandlePlayerRealtime()
{
	if (player != nullptr)
		player->HandleRealtime();
}

void World::SetPlayerControlEnabled(bool enabled) noexcept
{
	if (player != nullptr)
		player->SetControlEnabled(enabled);
}

void World::SpawnPlayerShot(const sf::Vector2f& pos, float rotation)
{
	++statistics.playerShotsFired;
	Spawn(std::make_unique<PlayerShot>(assets, *this, pos, rotation));
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
					AddEffectEvent({ EffectEventType::PlayerHit,
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
		AddEffectEvent({
			entity.GetType() == Entity::Type::Asteroid
				? EffectEventType::AsteroidHit
				: EffectEventType::ShipHit,
			enemy.GetPosition(), direction,
			std::clamp(enemy.GetCollisionRadius() / 45.f, 0.65f, 1.25f) });
		const bool killed{ enemy.TakeDamage(damage) };
		enemy.ApplyImpulse(direction * impulse);
		if (killed)
			AwardScore(enemy);
	}
}

void World::AddSound(Config::Sound id, float pitch)
{
	audio.PlaySound(id, SoundGroup::Gameplay, 100.f, pitch);
}

void World::AddEffectEvent(const EffectEvent& event)
{
	effectEvents.push_back(event);
}

const std::vector<World::EffectEvent>& World::GetEffectEvents() const noexcept
{
	return effectEvents;
}

void World::ClearEffectEvents() noexcept
{
	effectEvents.clear();
}

void World::PauseActiveSounds() { audio.PauseSounds(SoundGroup::Gameplay); }
void World::ResumePausedSounds() { audio.ResumeSounds(SoundGroup::Gameplay); }
void World::StopActiveSounds() { audio.StopSounds(SoundGroup::Gameplay); }

void World::ClearProjectiles()
{
	const auto isProjectile{ [](const auto& entity)
	{
		return entity->GetType() == Entity::Type::Projectile_Player ||
			entity->GetType() == Entity::Type::Projectile_Enemy ||
			entity->GetType() == Entity::Type::EnemyMissile;
	} };
	std::erase_if(entities, isProjectile);
	std::erase_if(pendingEntities, isProjectile);
}

void World::ClearPickups()
{
	const auto isPickup{ [](const auto& entity)
	{
		return entity->GetType() == Entity::Type::Pickup;
	} };
	std::erase_if(entities, isPickup);
	std::erase_if(pendingEntities, isPickup);
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

bool World::IsEntityActive(const Entity* entity) const noexcept
{
	return entity != nullptr && std::ranges::any_of(entities,
		[entity](const auto& candidate)
		{
			return candidate.get() == entity && candidate->IsAlive();
		});
}

std::optional<World::PlayerEffectState> World::GetPlayerEffectState() const
{
	if (player == nullptr || !player->IsAlive())
		return std::nullopt;

	return PlayerEffectState{
		player->GetEngineEmitterPositions(),
		player->GetVelocity(),
		player->GetExhaustDirection(),
		player->IsThrusting() };
}

std::optional<sf::Vector2f> World::GetPlayerGamepadAimPoint() const
{
	return player != nullptr ? player->GetGamepadAimPoint() : std::nullopt;
}

unsigned int World::GetWidth() const noexcept { return width; }
unsigned int World::GetHeight() const noexcept { return height; }
GameplaySession& World::GetSession() noexcept { return session; }
const World::Statistics& World::GetStatistics() const noexcept { return statistics; }

void World::Clear()
{
	entities.clear();
	pendingEntities.clear();
	effectEvents.clear();
	player = nullptr;
	shieldVisualTime = 0.f;
	statistics = {};
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
	for (std::size_t i{ 0 }; i < entities.size(); ++i)
	{
		for (std::size_t j{ i + 1 }; j < entities.size(); ++j)
		{
			Entity& first{ *entities[i] };
			Entity& second{ *entities[j] };
			if (first.IsAlive() && second.IsAlive() &&
				first.IsCollideWith(second) && second.IsCollideWith(first))
			{
				HandleCollisionPair(first, second);
			}
		}
	}
}

void World::HandleCollisionPair(Entity& first, Entity& second)
{
	if (first.GetType() == Entity::Type::Pickup || second.GetType() == Entity::Type::Pickup)
	{
		Pickup& pickup{ static_cast<Pickup&>(
			first.GetType() == Entity::Type::Pickup ? first : second) };
		Entity& other{ first.GetType() == Entity::Type::Pickup ? second : first };
		if (other.GetType() == Entity::Type::Player && pickup.Apply(session))
		{
			AddSound(Config::Sound::BonusTouched);
			if (pickup.GetKind() == Pickup::Kind::Shield)
				++statistics.shieldPickupsCollected;
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
		if (other.GetType() == Entity::Type::Projectile_Player)
		{
			++statistics.playerShotsHit;
			auto& shot{ static_cast<Shot&>(other) };
			AddEffectEvent({ EffectEventType::ShipHit,
				missile.GetPosition(), Normalize(shot.GetVelocity()), 0.55f });
			shot.Destroy();
			if (!missile.TakeDamage(shot.GetDamage()))
				AddSound(Config::Sound::MetalHit, 1.2f);
		}
		else
		{
			missile.Detonate();
			if (other.GetType() == Entity::Type::EnemyMissile)
				static_cast<HomingMissile&>(other).Detonate();
		}
		return;
	}

	auto handlePlayerShot = [this](Shot& shot, Enemy& enemy)
	{
		++statistics.playerShotsHit;
		const sf::Vector2f impactDirection{ Normalize(shot.GetVelocity()) };
		AddEffectEvent({
			enemy.GetType() == Entity::Type::Asteroid
				? EffectEventType::AsteroidHit
				: EffectEventType::ShipHit,
			shot.GetPosition(),
			impactDirection,
			std::clamp(enemy.GetCollisionRadius() / 45.f, 0.55f, 1.15f) });
		shot.Destroy();
		const bool killed{ enemy.TakeDamage(shot.GetDamage()) };
		enemy.ApplyImpulse(impactDirection * shot.GetKnockback());
		if (killed)
			AwardScore(enemy);
		else if (enemy.GetType() == Entity::Type::Asteroid)
			AddSound(Config::Sound::BulletHitAsteroid, enemy.GetSoundPitch());
		else if (enemy.GetType() == Entity::Type::Enemy)
			AddSound(Config::Sound::MetalHit);
	};

	if (first.GetType() == Entity::Type::Projectile_Player)
	{
		handlePlayerShot(static_cast<Shot&>(first), static_cast<Enemy&>(second));
		return;
	}
	if (second.GetType() == Entity::Type::Projectile_Player)
	{
		handlePlayerShot(static_cast<Shot&>(second), static_cast<Enemy&>(first));
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
					AddEffectEvent({ EffectEventType::PlayerHit,
						shot.GetPosition(), Normalize(shot.GetVelocity()), 1.f });
				targetPlayer.ApplyImpulse(Normalize(shot.GetVelocity()) * shot.GetKnockback());
			}
			if (damageAccepted && targetPlayer.IsAlive())
				AddSound(Config::Sound::MetalHit);
			if (!targetPlayer.IsAlive())
				session.SetGameOver();
		}
		else
		{
			auto& enemy{ static_cast<Enemy&>(target) };
			const bool destroyed{ enemy.TakeDamage(shot.GetDamage()) };
			(void)destroyed;
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
			AddEffectEvent({ EffectEventType::PlayerHit,
				impactPosition, -impactDirection, 1.15f });
		AddEffectEvent({
			collidedEnemy->GetType() == Entity::Type::Asteroid
				? EffectEventType::AsteroidHit
				: EffectEventType::ShipHit,
			impactPosition,
			impactDirection,
			1.1f });

		const Config::Sound impactSound{ collidedEnemy->GetType() == Entity::Type::Asteroid
			? Config::Sound::HitAsteroid
			: Config::Sound::HitEnemySaucer };
		AddSound(impactSound, collidedEnemy->GetSoundPitch());

		const bool enemyKilled{ collidedEnemy->TakeDamage(
			assets.GetGameplayData().GetPlayer().collisionDamage) };
		if (enemyKilled)
			AwardScore(*collidedEnemy);
	}

	if (!collidedPlayer->IsAlive())
		session.SetGameOver();

	if (!collidedPlayer->IsAlive() || !collidedEnemy->IsAlive())
		return;

	const auto manifold{ collidedPlayer->GetCollisionManifold(*collidedEnemy) };
	if (!manifold)
		return;

	ResolveCollision(*collidedPlayer, *collidedEnemy,
		manifold->normal, manifold->penetration);
	if (damageAccepted)
	{
		collidedPlayer->ApplyImpulse(-manifold->normal * collidedEnemy->GetCollisionImpulse());
		collidedEnemy->ApplyImpulse(manifold->normal *
			assets.GetGameplayData().GetPlayer().collisionImpulse);
	}
}

void World::AwardScore(const Enemy& enemy)
{
	if (const auto* meteor{ dynamic_cast<const Meteor*>(&enemy) })
	{
		if (meteor->GetSize() == Meteor::Size::Big)
			++statistics.bigMeteorsDestroyed;
		else
			++statistics.smallMeteorsDestroyed;
	}
	else if (const auto* saucer{ dynamic_cast<const Saucer*>(&enemy) };
		saucer != nullptr && saucer->GetMode() == Saucer::Mode::Shooter)
	{
		++statistics.shootersDestroyed;
	}

	const int points{ enemy.GetScoreValue() };
	session.AddScore(points);
	AddEffectEvent({
		EffectEventType::ScorePopup,
		enemy.GetPosition(),
		{},
		1.f,
		points });

	if (const auto pickupKind{ enemy.RollPickupDrop() })
	{
		auto pickup{ std::make_unique<Pickup>(assets, *this, *pickupKind) };
		pickup->SetPosition(enemy.GetPosition());
		Spawn(std::move(pickup));
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
	for (const auto& entity : entities)
	{
		if (entity->GetType() == Entity::Type::Pickup)
			DrawPickupAura(target, static_cast<const Pickup&>(*entity), shieldVisualTime, states);

		target.draw(*entity, states);
		if (entity->GetType() == Entity::Type::Enemy ||
			entity->GetType() == Entity::Type::EnemyMissile)
		{
			sf::RenderStates emissionStates{ states };
			emissionStates.shader = &enemyEmissionShader;
			emissionStates.blendMode = sf::BlendAdd;
			target.draw(entity->GetSprite(), emissionStates);
		}
		else if (entity->GetType() == Entity::Type::Player)
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
	}
}
