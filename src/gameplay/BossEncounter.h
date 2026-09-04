#pragma once

#include <array>
#include <functional>
#include <optional>
#include <vector>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "gameplay/GameplayData.h"

class Assets;
class LocalizationManager;
class World;

class BossEncounter final : public sf::Drawable
{
public:
	using SpawnReinforcement =
		std::function<void(GameplayData::EnemyKind, std::optional<sf::Vector2f>, std::optional<GameplayData::PickupKind>)>;

	BossEncounter(Assets& assets, LocalizationManager& localization, sf::Vector2f logicalSize);

	// Lifecycle: Reset() rewinds to Dormant (level (re)start), Start() begins
	// the arrival sequence, Update() drives everything once per frame.
	void Reset();
	void Start();
	void Update(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement);

	// HUD (health bar + armor label). Separate from the boss sprite itself,
	// which draws through the inherited sf::Drawable::draw() below.
	void DrawHUD(sf::RenderTarget& target) const;

	// State queries GameplayState reacts to.
	[[nodiscard]] bool IsActive() const noexcept;
	[[nodiscard]] bool IsReady() const noexcept;
	[[nodiscard]] bool IsDefeatSequenceActive() const noexcept;
	[[nodiscard]] bool IsVictoryReady() const noexcept;

private:
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
	void UpdateArmorLabel();
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

	// --- Arena layout & visuals -------------------------------------------
	sf::Vector2f startPosition;
	sf::Vector2f battlePosition;
	sf::Vector2f arenaSize;
	sf::Vector2f position;
	sf::Sprite outerRing;
	sf::Sprite diamond;
	sf::Sprite core;
	sf::Text armorLabel;
	float hudCenterX = 0.f;

	// --- Overall state machine ---------------------------------------------
	State state = State::Dormant;
	float stateElapsed = 0.f;
	int health = 0;
	float shieldPulse = 0.f;

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
	std::array<sf::CircleShape, 4> destroyedPortalMasks;
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

	// --- Shared references / cross-phase visuals -----------------------------------
	float hitFlashDuration = 0.f;
	LocalizationManager& localization;
	sf::Shader& hitFlashShader;
	const GameplayData::BossConfig& config;

	struct Lightning
	{
		sf::Vector2f start;
		sf::Vector2f end;
		float elapsed = 0.f;
	};
	std::vector<Lightning> lightning;
};
