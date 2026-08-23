#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "Entity.h"
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
		float width{ 0.f };
		int damage{ 0 };
		std::uint64_t attackID{ 0u };
	};

	enum class EffectEventType
	{
		PlayerProjectileGlow,
		PlayerHomingProjectileGlow,
		PlayerTripleProjectileGlow,
		EnemyProjectileGlow,
		MissileSmoke,
		EnemyEngine,
		StationWelding,
		StationChainExplosion,
		PlayerMuzzleFlash,
		EnemyMuzzleFlash,
		AsteroidHit,
		ShipHit,
		PlayerHit,
		AsteroidExplosion,
		ShipExplosion,
		StationExplosion,
		PlayerTeleport,
		BossDestructionShake,
		ScorePopup
	};

	struct EffectEvent
	{
		EffectEventType type;
		sf::Vector2f position;
		sf::Vector2f direction;
		float scale{ 1.f };
		int value{ 0 };
	};

	struct PlayerEffectState
	{
		std::array<sf::Vector2f, 2> enginePositions;
		sf::Vector2f velocity;
		sf::Vector2f exhaustDirection;
		bool isThrusting{ false };
	};

	struct PlayerProjectileImpact
	{
		sf::Vector2f position;
		int damage{ 0 };
	};

	struct Statistics
	{
		unsigned int playerAttacksFired{ 0u };
		unsigned int playerAttacksHit{ 0u };
		unsigned int bigMeteorsDestroyed{ 0u };
		unsigned int smallMeteorsDestroyed{ 0u };
		unsigned int shootersDestroyed{ 0u };
		unsigned int shieldPickupsCollected{ 0u };
	};

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
	[[nodiscard]] std::optional<PlayerLaserDamageEvent>
		ConsumePlayerLaserDamageEvent() noexcept;
	void SpawnSaucerShot(
		const sf::Vector2f& pos,
		const sf::Vector2f& target,
		GameplayData::ProjectileKind projectileKind = GameplayData::ProjectileKind::Enemy,
		bool playSound = true);
	void SpawnHomingMissile(const sf::Vector2f& pos, const sf::Vector2f& target);
	void SpawnStationShooter(
		const sf::Vector2f& position,
		float materializationDuration,
		const Entity* station);
	void DamagePlayerWithBeam(
		const sf::Vector2f& start,
		const sf::Vector2f& end,
		float width,
		int damage);
	void ExplodeEnemyMissile(
		const sf::Vector2f& position,
		float radius,
		int damage,
		float impulse);

	std::uint64_t AddSound(Config::Sound id, float pitch = 1.f);
	std::uint64_t AddSustainedSound(
		Config::Sound id,
		float pitch,
		float loopStartSeconds,
		float loopEndSeconds,
		float outroStartSeconds);
	void StopSound(std::uint64_t handle);
	void CompleteDelayedEnemyDestruction(Enemy& enemy);
	void AddEffectEvent(const EffectEvent& event);
	[[nodiscard]] const std::vector<EffectEvent>& GetEffectEvents() const noexcept;
	void ClearEffectEvents() noexcept;
	void PauseActiveSounds();
	void ResumePausedSounds();
	void StopActiveSounds();
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
	void KeepPlayerOutsideCircle(
		sf::Vector2f center, float radius, int contactDamage);
	void KeepEnemiesOutsideCircle(
		sf::Vector2f center, float radius, float clearance);
	void SetRewardExclusionCircle(sf::Vector2f center, float radius) noexcept;
	[[nodiscard]] std::size_t CountActiveLaserTurrets() const noexcept;
	[[nodiscard]] bool DestroyNextBossVictoryTarget();
	void DestroyAllBossVictoryTargets();
	void ClearBossVictoryPickupsAndCompanions();
	void ClearPickups();
	void ConfigureCampaignPickupSequence(
		const std::vector<GameplayData::PickupKind>& sequence);

	[[nodiscard]] sf::Vector2f GetPlayerPosition() const noexcept;
	[[nodiscard]] const Entity* FindHomingTarget(
		const sf::Vector2f& position,
		const sf::Vector2f& direction,
		float minimumDirectionDot) const noexcept;
	void SetBossHomingTargets(
		const std::array<std::optional<sf::Vector2f>, 4>& targets) noexcept;
	void ClearBossHomingTargets() noexcept;
	[[nodiscard]] std::optional<std::size_t> FindBossHomingTarget(
		const sf::Vector2f& position,
		const sf::Vector2f& direction,
		float minimumDirectionDot) const noexcept;
	[[nodiscard]] std::optional<sf::Vector2f> GetBossHomingTargetPosition(
		std::size_t index) const noexcept;
	[[nodiscard]] bool IsEntityActive(const Entity* entity) const noexcept;
	[[nodiscard]] std::optional<PlayerEffectState> GetPlayerEffectState() const;
	[[nodiscard]] std::optional<sf::Vector2f> GetPlayerGamepadAimPoint() const;
	[[nodiscard]] unsigned int GetWidth() const noexcept;
	[[nodiscard]] unsigned int GetHeight() const noexcept;
	[[nodiscard]] GameplaySession& GetSession() noexcept;
	[[nodiscard]] const Statistics& GetStatistics() const noexcept;

	sf::RenderWindow& GetWindow() noexcept;
	void SetWindow(sf::RenderWindow& window);

	void Clear();
	bool IsCleared() const noexcept;

	bool HasPlayer() const noexcept;
	void SpawnPlayer(Assets& assets, InputHandler<Config::PlayerAction>& input);
	void SetPlayerSpawnPresentation(float progress) noexcept;
	void TeleportPlayerToCenter() noexcept;

	void HandlePlayerEvent(const sf::Event& event);
	void HandlePlayerRealtime();
	void SetPlayerControlEnabled(bool enabled) noexcept;
	void SetPlayerFiringEnabled(bool enabled) noexcept;
	void SetPlayerCinematicInvulnerable(bool enabled) noexcept;

private:
	void Wrap(Entity& e) const;
	void HandleCollisions();
	void ResolveCollision(Entity& first, Entity& second,
		const sf::Vector2f& normal, float penetration) const;
	void HandleCollisionPair(Entity& first, Entity& second);
	void AwardScore(const Enemy& enemy);
	void RemoveDeadEntities();
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	std::vector<std::unique_ptr<Entity>> entities;
	std::vector<std::unique_ptr<Entity>> pendingEntities;
	std::vector<EffectEvent> effectEvents;
	std::optional<sf::Vector2f> rewardExclusionCenter;
	std::array<std::optional<sf::Vector2f>, 4> bossHomingTargets{};
	std::optional<PlayerLaserDamageEvent> playerLaserDamageEvent;
	float rewardExclusionRadius{ 0.f };
	Assets& assets;
	AudioManager& audio;
	GameplaySession& session;
	GamepadManager& gamepad;

	Player* player{ nullptr };
	sf::RenderWindow* window{ nullptr };

	unsigned int width;
	unsigned int height;
	float shieldVisualTime{ 0.f };
	Statistics statistics;
	std::uint64_t nextPlayerAttackID{ 1u };
	std::unordered_set<std::uint64_t> successfulPlayerAttacks;
	bool helperPickupSpawned{ false };
	bool helperBotSpawned{ false };
	std::vector<GameplayData::PickupKind> campaignPickupSequence;
	std::size_t nextCampaignPickup{ 0u };
};
