#pragma once

#include <array>
#include <memory>
#include <optional>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "Entity.h"
#include "game/GameplayData.h"
#include "utils/ConfigEnums.h"
#include "systems/InputHandler.h"

class AssetStore;
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
	enum class EffectEventType
	{
		PlayerProjectileGlow,
		PlayerHomingProjectileGlow,
		EnemyProjectileGlow,
		MissileSmoke,
		EnemyEngine,
		PlayerMuzzleFlash,
		EnemyMuzzleFlash,
		AsteroidHit,
		ShipHit,
		PlayerHit,
		AsteroidExplosion,
		ShipExplosion,
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

	struct Statistics
	{
		unsigned int playerShotsFired{ 0u };
		unsigned int playerShotsHit{ 0u };
		unsigned int bigMeteorsDestroyed{ 0u };
		unsigned int smallMeteorsDestroyed{ 0u };
		unsigned int shootersDestroyed{ 0u };
		unsigned int shieldPickupsCollected{ 0u };
	};

	World(unsigned int width, unsigned int height, AssetStore& assets, AudioManager& audio,
		GameplaySession& session, GamepadManager& gamepad);

	void Update(float deltaTime, float worldTimeScale = 1.f);
	void CommitPendingEntities();

	void Spawn(std::unique_ptr<Entity> entity);
	void SpawnPlayerShot(const sf::Vector2f& pos, float rotation);
	void SpawnSaucerShot(
		const sf::Vector2f& pos,
		const sf::Vector2f& target,
		GameplayData::ProjectileKind projectileKind = GameplayData::ProjectileKind::Enemy,
		bool playSound = true);
	void SpawnHomingMissile(const sf::Vector2f& pos, const sf::Vector2f& target);
	void ExplodeEnemyMissile(
		const sf::Vector2f& position,
		float radius,
		int damage,
		float impulse);

	void AddSound(Config::Sound id, float pitch = 1.f);
	void AddEffectEvent(const EffectEvent& event);
	[[nodiscard]] const std::vector<EffectEvent>& GetEffectEvents() const noexcept;
	void ClearEffectEvents() noexcept;
	void PauseActiveSounds();
	void ResumePausedSounds();
	void StopActiveSounds();
	void ClearProjectiles();
	void ClearPickups();

	[[nodiscard]] sf::Vector2f GetPlayerPosition() const noexcept;
	[[nodiscard]] const Entity* FindHomingTarget(
		const sf::Vector2f& position,
		const sf::Vector2f& direction,
		float minimumDirectionDot) const noexcept;
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
	void SpawnPlayer(AssetStore& assets, InputHandler<Config::PlayerAction>& input);
	void SetPlayerSpawnPresentation(float progress) noexcept;

	void HandlePlayerEvent(const sf::Event& event);
	void HandlePlayerRealtime();
	void SetPlayerControlEnabled(bool enabled) noexcept;

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
	AssetStore& assets;
	AudioManager& audio;
	GameplaySession& session;
	GamepadManager& gamepad;

	Player* player{ nullptr };
	sf::RenderWindow* window{ nullptr };

	unsigned int width;
	unsigned int height;
	float shieldVisualTime{ 0.f };
	Statistics statistics;
};
