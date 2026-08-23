#pragma once

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <array>
#include <functional>
#include <optional>
#include <vector>

#include "gameplay/GameplayData.h"

class Assets;
class LocalizationManager;
class World;

class BossEncounter final : public sf::Drawable
{
public:
	using SpawnReinforcement = std::function<void(
		GameplayData::EnemyKind,
		std::optional<sf::Vector2f>,
		std::optional<GameplayData::PickupKind>)>;

	BossEncounter(Assets& assets, LocalizationManager& localization, sf::Vector2f logicalSize);

	void Reset();
	void Start();
	void Update(float deltaTime, World& world,
		const SpawnReinforcement& spawnReinforcement);
	void DrawHud(sf::RenderTarget& target) const;

	[[nodiscard]] bool IsActive() const noexcept;
	[[nodiscard]] bool IsReady() const noexcept;
	[[nodiscard]] bool IsDefeatSequenceActive() const noexcept;
	[[nodiscard]] bool IsVictoryReady() const noexcept;
#ifdef _DEBUG
	void DebugAdvancePhase(World& world);
	void DebugDefeat(World& world);
#endif

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

	void SetPosition(sf::Vector2f position);
	void UpdateShield(float deltaTime);
	void UpdateLightning(float deltaTime);
	void HandleShieldImpacts(
		World& world, sf::Vector2f center, float shieldRadius);
	void UpdateOuterRingMechanics(float deltaTime, World& world);
	void UpdateOuterShieldWarning(float deltaTime, World& world);
	void UpdateOuterShield(float deltaTime, World& world,
		const SpawnReinforcement& spawnReinforcement);
	void BeginOuterShield(int cycle);
	void BeginOuterRingDestruction();
	void UpdateOuterRingDestruction(float deltaTime, World& world);
	void HandleOuterRingImpacts(World& world);
	void UpdateInnerPhase(float deltaTime, World& world,
		const SpawnReinforcement& spawnReinforcement, bool shielded);
	void HandlePortalImpacts(World& world);
	void BeginInnerShield();
	void BeginDiamondDestruction();
	void UpdateDiamondDestruction(float deltaTime, World& world);
	void UpdateCorePhase(float deltaTime, World& world,
		const SpawnReinforcement& spawnReinforcement);
	void BeginCoreShieldCycle(int cycle,
		const SpawnReinforcement& spawnReinforcement,
		bool useWarning);
	void SpawnCoreTurrets(const SpawnReinforcement& spawnReinforcement);
	void SpawnCoreStations(const SpawnReinforcement& spawnReinforcement);
	void HandleCoreImpacts(World& world,
		const SpawnReinforcement& spawnReinforcement);
	void HandleCoreLaser(World& world,
		const SpawnReinforcement& spawnReinforcement);
	void ApplyCoreDamage(int damage, World& world,
		const SpawnReinforcement& spawnReinforcement);
	void UpdateCoreDestruction(float deltaTime, World& world);
	[[nodiscard]] sf::Vector2f GetCoreBeamEnd() const;
	[[nodiscard]] sf::Vector2f GetPortalPosition(std::size_t index) const;
	[[nodiscard]] sf::Vector2f GetSafePickupPosition(std::size_t quadrantIndex) const;
	[[nodiscard]] float GetShieldRadius() const noexcept;
	[[nodiscard]] float GetEnemyExclusionRadius() const noexcept;
	[[nodiscard]] sf::Vector2f GetCollisionCenter() const noexcept;
	[[nodiscard]] float GetPlayerExclusionRadius() const noexcept;
	[[nodiscard]] bool IsShieldCollisionActive() const noexcept;
	void UpdateArmorLabel();
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	static constexpr float ArrivalDuration{ 4.f };
	static constexpr float ShieldDelayDuration{ 2.f };

	sf::Vector2f startPosition;
	sf::Vector2f battlePosition;
	sf::Vector2f arenaSize;
	sf::Sprite outerRing;
	sf::Sprite diamond;
	sf::Sprite core;
	sf::Text armorLabel;
	sf::Vector2f position;
	State state{ State::Dormant };
	float stateElapsed{ 0.f };
	float shieldPulse{ 0.f };
	int health{ 0 };
	float ringRotationDegrees{ 0.f };
	float phaseElapsed{ 0.f };
	float reinforcementElapsed{ 0.f };
	float nextKamikazeSpawn{ 0.f };
	float nextShooterSpawn{ 0.f };
	float destructionExplosionElapsed{ 0.f };
	float diamondRotationDegrees{ 0.f };
	float nextPortalSpawn{ 0.f };
	float nextEdgeShooterSpawn{ 0.f };
	float coreBeamAngleDegrees{ -90.f };
	float coreBeamVisualTime{ 0.f };
	float nextCoreAsteroidSpawn{ 0.f };
	float ringHitFlashRemaining{ 0.f };
	float diamondHitFlashRemaining{ 0.f };
	float coreHitFlashRemaining{ 0.f };
	float coreExplosionElapsed{ 0.f };
	float victoryCleanupElapsed{ 0.f };
	float hitFlashDuration{ 0.f };
	float hudCenterX{ 0.f };
	int shieldCycle{ 0 };
	int coreCycle{ 0 };
	bool corePhaseInitialized{ false };
	bool outerRingVisible{ true };
	bool diamondVisible{ true };
	bool coreVisible{ true };
	std::size_t nextPortalIndex{ 0u };
	std::size_t nextPhaseOneBonus{ 0u };
	std::size_t nextPhaseTwoBonus{ 0u };
	std::size_t nextPhaseThreeBonus{ 0u };
	std::array<int, 4> portalHealth{};
	std::array<sf::CircleShape, 4> destroyedPortalMasks;
	std::vector<float> nextCannonShots;
	LocalizationManager& localization;
	sf::Shader& hitFlashShader;
	const GameplayData::BossConfig& config;

	struct Lightning
	{
		sf::Vector2f start;
		sf::Vector2f end;
		float elapsed{ 0.f };
	};
	std::vector<Lightning> lightning;
};
