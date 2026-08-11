#include "World.h"

#include <algorithm>
#include <cmath>
#include <SFML/Graphics/RenderWindow.hpp>
#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "entities/Enemy.h"
#include "entities/Player.h"
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
}

World::World(unsigned int width, unsigned int height, AssetStore& assets,
	AudioManager& audio, GameplaySession& session, GamepadManager& gamepadManager)
	: assets(assets), audio(audio), session(session), gamepad(gamepadManager),
	  width(width), height(height)
{
	effectEvents.reserve(256);
}

void World::Update(float deltaTime)
{
	if (!pendingEntities.empty())
	{
		entities.insert(entities.end(),
			std::make_move_iterator(pendingEntities.begin()),
			std::make_move_iterator(pendingEntities.end()));
		pendingEntities.clear();
	}

	for (auto& entity : entities)
	{
		entity->UpdateEffects(deltaTime);
		entity->Update(deltaTime);
		if (entity->GetType() != Entity::Type::Projectile_Player &&
			entity->GetType() != Entity::Type::Projectile_Enemy)
		{
			Wrap(*entity);
		}
	}

	HandleCollisions();
	RemoveDeadEntities();
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

void World::SpawnPlayerShot(const sf::Vector2f& pos, float rotation)
{
	Spawn(std::make_unique<PlayerShot>(assets, *this, pos, rotation));
}

void World::SpawnSaucerShot(const sf::Vector2f& pos, const sf::Vector2f& target)
{
	Spawn(std::make_unique<SaucerShot>(assets, *this, pos, target));
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

sf::Vector2f World::GetPlayerPosition() const noexcept
{
	return player != nullptr ? player->GetPosition() : sf::Vector2f{};
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

void World::Clear()
{
	entities.clear();
	pendingEntities.clear();
	effectEvents.clear();
	player = nullptr;
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
	auto handlePlayerShot = [this](Shot& shot, Enemy& enemy)
	{
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
	const int points{ enemy.GetScoreValue() };
	session.AddScore(points);
	AddEffectEvent({
		EffectEventType::ScorePopup,
		enemy.GetPosition(),
		{},
		1.f,
		points });
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
	for (const auto& entity : entities)
		target.draw(*entity, states);
}
