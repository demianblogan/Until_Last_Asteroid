#pragma once

#include <array>
#include <functional>
#include <optional>
#include <vector>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/System/Vector2.hpp>

#include "gameplay/BossVisual.h"
#include "gameplay/GameplayData.h"

class Assets;
class LocalizationManager;
class World;

class BossEncounter final : public sf::Drawable
{
public:
	using SpawnReinforcement =
		std::function<void(GameplayData::EnemyKind, std::optional<sf::Vector2f>, std::optional<GameplayData::PickupKind>)>;

	// The core sprite sits a bit above the boss's logical position, and the
	// retaliation-lightning bolts fade out over this many seconds. Both are
	// needed by BossVisual too, hence public.
	static constexpr sf::Vector2f CoreVisualOffset{ 0.f, -37.f };
	static constexpr float LightningDuration = 0.32f;

	BossEncounter(Assets& assets, LocalizationManager& localization, sf::Vector2f logicalSize);

	// Lifecycle: Reset() rewinds to Dormant (level (re)start), Start() begins
	// the arrival sequence, Update() drives everything once per frame.
	void Reset();
	void Start();
	void Update(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement);

	// HUD (health bar + armor label). Separate from the boss sprite itself,
	// which draws through the inherited sf::Drawable::draw() below. Both just
	// forward to the BossVisual member.
	void DrawHUD(sf::RenderTarget& target) const;

	// State queries GameplayState reacts to.
	[[nodiscard]] bool IsActive() const noexcept;
	[[nodiscard]] bool IsReady() const noexcept;
	[[nodiscard]] bool IsDefeatSequenceActive() const noexcept;
	[[nodiscard]] bool IsVictoryReady() const noexcept;

private:
	// BossVisual reads this class's state directly (state enum, positions,
	// hit-flash timers, ...) to render the boss -- the two are a tightly
	// coupled pair, so friendship rather than a wide public accessor surface.
	friend class BossVisual;

	enum class State
	{
		Dormant,
		Arriving,
		ShieldDelay,
		OuterExposed,
		OuterShieldWarning,
		OuterShield,
		OuterDestroying,
		InnerPhase,
		InnerShieldWarning,
		InnerShield,
		InnerDestroying,
		CoreShieldWarning,
		CoreShield,
		CoreExposed,
		CoreDying,
		VictorySilence,
		VictoryReady
	};

	// A single retaliation bolt: a jagged line drawn from where a player shot
	// hit the boss's shield back to the player, fading over LightningDuration.
	struct Lightning
	{
		sf::Vector2f start;
		sf::Vector2f end;
		float elapsed = 0.f;
	};

	// --- Shared across every phase --------------------------------------
	void SetPosition(sf::Vector2f position);
	void UpdateShield(float deltaTime);
	void UpdateLightning(float deltaTime);
	void HandleShieldImpacts(World& world, sf::Vector2f center, float shieldRadius);
	[[nodiscard]] float GetShieldRadius() const noexcept;
	[[nodiscard]] bool IsShieldCollisionActive() const noexcept;
	[[nodiscard]] sf::Vector2f GetCollisionCenter() const noexcept;
	[[nodiscard]] float GetEnemyExclusionRadius() const noexcept;
	[[nodiscard]] float GetPlayerExclusionRadius() const noexcept;
	[[nodiscard]] sf::Vector2f GetSafePickupPosition(std::size_t quadrantIndex) const;

	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	// --- Phase 1: outer ring ---------------------------------------------
	void UpdateOuterRingMechanics(float deltaTime, World& world);
	void UpdateOuterShieldWarning(float deltaTime, World& world);
	void UpdateOuterShield(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement);
	void BeginOuterShield(int cycle);
	void BeginOuterRingDestruction();
	void UpdateOuterRingDestruction(float deltaTime, World& world);
	void HandleOuterRingImpacts(World& world);

	// --- Phase 2: inner ring / diamond / portals -------------------------
	void UpdateInnerPhase(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement, bool isShielded);
	void HandlePortalImpacts(World& world);
	void BeginInnerShield();
	void BeginDiamondDestruction();
	void UpdateDiamondDestruction(float deltaTime, World& world);
	[[nodiscard]] sf::Vector2f GetPortalPosition(std::size_t index) const;

	// --- Phase 3: core ----------------------------------------------------
	void UpdateCorePhase(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement);
	void BeginCoreShieldCycle(int cycle, const SpawnReinforcement& spawnReinforcement, bool needToShowWarning);
	void SpawnCoreTurrets(const SpawnReinforcement& spawnReinforcement);
	void SpawnCoreStations(const SpawnReinforcement& spawnReinforcement);
	void HandleCoreImpacts(World& world, const SpawnReinforcement& spawnReinforcement);
	void HandleCoreLaser(World& world, const SpawnReinforcement& spawnReinforcement);
	void ApplyCoreDamage(int damage, World& world, const SpawnReinforcement& spawnReinforcement);
	void UpdateCoreDestruction(float deltaTime, World& world);
	[[nodiscard]] sf::Vector2f GetCoreBeamEnd() const;

	static constexpr float ArrivalDuration = 4.f;
	static constexpr float ShieldDelayDuration = 2.f;

	// How long the core's death animation plays before the fight goes quiet
	// (VictorySilence), and how long that silence itself lasts before the
	// level is reported complete (VictoryReady). Two different waits that
	// happen to share a duration, not one value used twice.
	static constexpr float CoreDeathAnimationDuration = 3.f;
	static constexpr float VictorySilenceDuration = 3.f;

	// How often the death sequence removes one more visible enemy/pickup
	// from the arena, and how often it spawns one more explosion around the
	// core -- siblings of outerRingExplosionInterval/diamondExplosionInterval
	// (which ARE in boss.json) that never got their own config entry.
	static constexpr float VictoryCleanupInterval = 0.1f;
	static constexpr float CoreExplosionInterval = 0.11f;

	// --- Arena layout ----------------------------------------------------
	sf::Vector2f startPosition;
	sf::Vector2f battlePosition;
	sf::Vector2f arenaSize;
	sf::Vector2f position;

	BossVisual visual;
	const GameplayData::BossConfig& config;

	// --- Overall state machine ---------------------------------------------
	State state = State::Dormant;
	float stateElapsed = 0.f;
	int health = 0;
	float shieldPulse = 0.f;
	float hitFlashDuration = 0.f;
	std::vector<Lightning> lightning;

	// --- Phase 1: outer ring state ------------------------------------------
	float ringRotationDegrees = 0.f;
	float phaseElapsed = 0.f;
	float reinforcementElapsed = 0.f;
	float nextKamikazeSpawn = 0.f;
	float nextShooterSpawn = 0.f;
	int shieldCycle = 0;
	bool isOuterRingVisible = true;
	float ringHitFlashRemaining = 0.f;
	std::vector<float> nextCannonShots;

	// --- Phase 2: inner ring / diamond / portals state ----------------------
	float diamondRotationDegrees = 0.f;
	float nextPortalSpawn = 0.f;
	float nextEdgeShooterSpawn = 0.f;
	bool isDiamondVisible = true;
	float diamondHitFlashRemaining = 0.f;
	std::array<int, 4> portalHealth{};
	std::size_t nextPortalIndex = 0u;

	// --- Phase 3: core state --------------------------------------------------
	float coreBeamAngleDegrees = -90.f;
	float coreBeamVisualTime = 0.f;
	float nextCoreAsteroidSpawn = 0.f;
	int coreCycle = 0;
	bool isCorePhaseInitialized = false;
	bool isCoreVisible = true;
	float coreHitFlashRemaining = 0.f;

	// --- Destruction / victory sequence ----------------------------------------
	// destructionExplosionElapsed is reused by both the outer-ring and the
	// diamond destruction sequences -- they never run at the same time.
	float destructionExplosionElapsed = 0.f;
	float coreExplosionElapsed = 0.f;
	float victoryCleanupElapsed = 0.f;

	// --- Bonus-pickup drop tracking (one counter per boss phase) -----------------
	std::size_t nextPhaseOneBonus = 0u;
	std::size_t nextPhaseTwoBonus = 0u;
	std::size_t nextPhaseThreeBonus = 0u;
};
