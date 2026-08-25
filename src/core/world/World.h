#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "core/Entity.h"
#include "WorldBossHomingTargets.h"
#include "WorldCampaignPickupQueue.h"
#include "WorldEffectEventQueue.h"
#include "WorldPlayerAttackTracker.h"
#include "WorldRewardExclusionZone.h"
#include "WorldSoundSystem.h"
#include "WorldStatisticsTracker.h"
#include "gameplay/GameplayData.h"
#include "utils/ConfigEnums.h"
#include "input/InputHandler.h"

class Assets;
class AudioManager;
class Enemy;
class GameplaySession;
class GamepadManager;
class HomingMissile;
class Player;

namespace sf
{
	class RenderTarget;
	struct RenderStates;
	class RenderWindow;
	class Event;
}

class Player;
class PlayerShot;
class SaucerShot;

class World : public sf::Drawable
{
public:
	struct PlayerLaserDamageEvent
	{
		sf::Vector2f start;
		sf::Vector2f end;
		float width = 0.f;
		int damage = 0;
		std::uint64_t attackID = 0u;
	};

	struct PlayerProjectileImpact
	{
		sf::Vector2f position;
		int damage = 0;
	};

	using Statistics = WorldStatistics;

	World(unsigned int width, unsigned int height, Assets& assets, AudioManager& audio,
		GameplaySession& session, GamepadManager& gamepad);

	void Update(float deltaTime, float worldTimeScale = 1.f);
	void CommitPendingEntities();

	void Spawn(std::unique_ptr<Entity> entity);
	[[nodiscard]] std::uint64_t BeginPlayerAttack() noexcept;
	void RegisterPlayerAttackHit(std::uint64_t attackID) noexcept;
	void SpawnPlayerShot(
		const sf::Vector2f& pos,
		float rotation,
		std::uint64_t attackID,
		bool playSound = true,
		bool tripleShotVisual = false);
	void SpawnHelperShot(const sf::Vector2f& pos, const Entity* target);
	[[nodiscard]] bool SpawnHelperPickup(const sf::Vector2f& pos);
	void SpawnHelperBot();
	void SpawnPickupAt(GameplayData::PickupKind kind, const sf::Vector2f& pos);
	void DamageEnemiesWithPlayerLaser(
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		float width,
		int damage,
		std::uint64_t attackID);
	[[nodiscard]] std::optional<PlayerLaserDamageEvent>	ConsumePlayerLaserDamageEvent() noexcept;
	void SpawnSaucerShot(
		const sf::Vector2f& pos,
		const sf::Vector2f& target,
		GameplayData::ProjectileKind projectileKind = GameplayData::ProjectileKind::Enemy,
		bool playSound = true);
	void SpawnHomingMissile(const sf::Vector2f& pos, const sf::Vector2f& target);
	void SpawnStationShooter(const sf::Vector2f& position, float materializationDuration, const Entity* station);
	void DamagePlayerWithBeam(const sf::Vector2f& start, const sf::Vector2f& end, float width, int damage);
	void ExplodeEnemyMissile(const sf::Vector2f& position, float radius, int damage, float impulse);

	[[nodiscard]] WorldSoundSystem& Sound() noexcept;
	[[nodiscard]] WorldEffectEventQueue& Effects() noexcept;
	void CompleteDelayedEnemyDestruction(Enemy& enemy);
	void ClearProjectiles();
	[[nodiscard]] std::vector<PlayerProjectileImpact> ConsumePlayerProjectilesInCircle(
		sf::Vector2f center, float radius);
	[[nodiscard]] std::vector<PlayerProjectileImpact> ConsumePlayerProjectilesInAnnulus(
		sf::Vector2f center, float innerRadius, float outerRadius);
	void ConsumePlayerProjectilesInDiamondFrame(
		sf::Vector2f center,
		float rotationDegrees,
		float vertexRadius,
		float halfThickness);
	bool DamagePlayerFromBoss(int damage, sf::Vector2f sourcePosition);
	void KeepPlayerOutsideCircle(sf::Vector2f center, float radius, int contactDamage);
	void KeepEnemiesOutsideCircle(sf::Vector2f center, float radius, float clearance);
	void SetRewardExclusionCircle(sf::Vector2f center, float radius) noexcept;
	[[nodiscard]] std::size_t CountActiveLaserTurrets() const noexcept;
	[[nodiscard]] bool DestroyNextBossVictoryTarget();
	void DestroyAllBossVictoryTargets();
	void ClearBossVictoryPickupsAndCompanions();
	void ClearPickups();
	void ConfigureCampaignPickupSequence(const std::vector<GameplayData::PickupKind>& sequence);

	[[nodiscard]] sf::Vector2f GetPlayerPosition() const noexcept;
	[[nodiscard]] const Entity* FindHomingTarget(
		const sf::Vector2f& position,
		const sf::Vector2f& direction,
		float minimumDirectionDot) const noexcept;
	[[nodiscard]] WorldBossHomingTargets& BossHomingTargets() noexcept;
	[[nodiscard]] bool IsEntityActive(const Entity* entity) const noexcept;
	[[nodiscard]] unsigned int GetWidth() const noexcept;
	[[nodiscard]] unsigned int GetHeight() const noexcept;
	[[nodiscard]] GameplaySession& GetSession() noexcept;
	[[nodiscard]] const Statistics& GetStatistics() const noexcept;

	void Clear();
	bool IsCleared() const noexcept;

	bool HasPlayer() const noexcept;
	[[nodiscard]] Player* GetPlayer() const noexcept;
	void SpawnPlayer(Assets& assets, InputHandler<Config::PlayerAction>& input, sf::RenderWindow& window);
	void SetPlayerSpawnPresentation(float progress) noexcept;
	void TeleportPlayerToCenter() noexcept;

private:
	void Wrap(Entity& e) const;
	void HandleCollisions();
	void ResolveCollision(Entity& first, Entity& second, const sf::Vector2f& normal, float penetration) const;
	void HandleCollisionPair(Entity& first, Entity& second);
	void AwardScore(const Enemy& enemy);
	void RemoveDeadEntities();
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	std::vector<std::unique_ptr<Entity>> entities;
	std::vector<std::unique_ptr<Entity>> pendingEntities;

	WorldEffectEventQueue effectEvents;

	WorldRewardExclusionZone rewardExclusionZone;

	WorldBossHomingTargets bossHomingTargets;
	std::optional<PlayerLaserDamageEvent> playerLaserDamageEvent;

	Assets& assets;
	GameplaySession& session;
	GamepadManager& gamepad;

	WorldSoundSystem sound;

	Player* player = nullptr;

	unsigned int width;
	unsigned int height;

	float shieldVisualTime = 0.f;

	WorldStatisticsTracker statisticsTracker;
	WorldPlayerAttackTracker playerAttackTracker;

	bool helperPickupSpawned = false;
	bool helperBotSpawned = false;

	WorldCampaignPickupQueue campaignPickupQueue;
};