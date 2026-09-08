#include "BossEncounter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

#include <SFML/System/Angle.hpp>

#include "assets/Assets.h"
#include "gameplay/VibrationProfiles.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Easing.h"
#include "utils/Random.h"
#include "utils/VectorMath.h"

namespace
{
	constexpr int ShieldRetaliationDamage = 10;
}

BossEncounter::BossEncounter(Assets& assets, LocalizationManager& localize, sf::Vector2f logicalSize)
	: startPosition{ logicalSize.x * 0.5f, -380.f }
	, battlePosition{ logicalSize.x * 0.5f, logicalSize.y * 0.43f }
	, arenaSize(logicalSize)
	, visual(assets, localize, logicalSize)
	, config(assets.GetGameplayData().GetBoss())
{
	hitFlashDuration = assets.GetGameplayData().GetHitFlashDuration();
	Reset();
}

void BossEncounter::Reset()
{
	state = State::Dormant;
	stateElapsed = 0.f;
	shieldPulse = 0.f;
	health = config.maximumHealth;

	ringRotationDegrees = 0.f;
	phaseElapsed = 0.f;
	reinforcementElapsed = 0.f;
	nextKamikazeSpawn = 0.f;
	nextShooterSpawn = 0.f;
	destructionExplosionElapsed = 0.f;

	diamondRotationDegrees = 0.f;
	nextPortalSpawn = 0.f;
	nextEdgeShooterSpawn = config.edgeShooterSpawnInterval;

	coreBeamAngleDegrees = -90.f;
	coreBeamVisualTime = 0.f;
	nextCoreAsteroidSpawn = config.coreAsteroidSpawnInterval;

	ringHitFlashRemaining = 0.f;
	diamondHitFlashRemaining = 0.f;
	coreHitFlashRemaining = 0.f;
	coreExplosionElapsed = 0.f;
	victoryCleanupElapsed = 0.f;

	shieldCycle = 0;
	coreCycle = 0;
	isCorePhaseInitialized = false;
	isOuterRingVisible = true;
	isDiamondVisible = true;
	isCoreVisible = true;

	nextPortalIndex = 0u;
	nextPhaseOneBonus = 0u;
	nextPhaseTwoBonus = 0u;
	nextPhaseThreeBonus = 0u;
	portalHealth.fill(config.portalHealth);

	nextCannonShots.resize(static_cast<std::size_t>(config.cannonCount));

	for (int index = 0; index < config.cannonCount; index++)
		nextCannonShots[static_cast<std::size_t>(index)] = static_cast<float>(index) * config.cannonStaggerInterval;

	lightning.clear();
	SetPosition(startPosition);
}

void BossEncounter::Start()
{
	state = State::Arriving;
	stateElapsed = 0.f;
	shieldPulse = 0.f;
	SetPosition(startPosition);
}

void BossEncounter::Update(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement)
{
	ringHitFlashRemaining = std::max(0.f, ringHitFlashRemaining - deltaTime);
	diamondHitFlashRemaining = std::max(0.f, diamondHitFlashRemaining - deltaTime);
	coreHitFlashRemaining = std::max(0.f, coreHitFlashRemaining - deltaTime);

	UpdateLightning(deltaTime);

	if (state == State::Dormant)
		return;

	world.BossHomingTargets().Clear();

	if (!IsDefeatSequenceActive())
		world.KeepPlayerOutsideCircle(GetCollisionCenter(), GetPlayerExclusionRadius(), config.contactDamage);

	if (IsReady())
	{
		world.KeepEnemiesOutsideCircle(GetCollisionCenter(), GetEnemyExclusionRadius(), config.shieldEnemyClearance);
		world.SetRewardExclusionCircle(GetCollisionCenter(), GetEnemyExclusionRadius() + config.shieldEnemyClearance);
	}

	if (state == State::Arriving || state == State::ShieldDelay)
		HandleShieldImpacts(world, position, config.outerShieldRadius);

	// One branch per state. Kept as an exhaustive switch with no default so
	// that adding a State enumerator without handling it here is a visible
	// gap (and a warning under /W4) rather than a silent no-op.
	switch (state)
	{
	case State::Dormant:          // already returned above; listed for completeness
	case State::VictoryReady:
		return;

	case State::Arriving:
	case State::ShieldDelay:
	{
		stateElapsed += deltaTime;
		UpdateShield(deltaTime);

		if (state == State::Arriving)
		{
			const float progress = Easing::SmoothStep(stateElapsed / ArrivalDuration);
			SetPosition(startPosition + (battlePosition - startPosition) * progress);

			if (stateElapsed >= ArrivalDuration)
			{
				SetPosition(battlePosition);
				state = State::ShieldDelay;
				stateElapsed = 0.f;
			}
		}
		else if (stateElapsed >= ShieldDelayDuration)
		{
			state = State::OuterExposed;
			stateElapsed = 0.f;
			phaseElapsed = 0.f;
		}

		return;
	}

	case State::OuterExposed:
	{
		UpdateOuterRingMechanics(deltaTime, world);
		HandleOuterRingImpacts(world);

		const float healthRatio = static_cast<float>(health) / static_cast<float>(config.maximumHealth);

		if (shieldCycle == 0 && healthRatio <= config.firstShieldHealthRatio)
		{
			BeginOuterShield(1);
		}
		else if (shieldCycle == 1 && healthRatio <= config.secondShieldHealthRatio)
		{
			BeginOuterShield(2);
		}
		else if (shieldCycle == 2 && healthRatio <= config.outerRingEndHealthRatio)
		{
			world.Effects().Add({ Rendering::EffectEventType::BossDestructionShake, position, {}, 8.f, 1000 });
			world.SpawnPickupAt(GameplayData::PickupKind::Health, GetSafePickupPosition(0u));
			BeginOuterRingDestruction();
		}
		return;
	}

	case State::OuterShield:
		UpdateOuterShield(deltaTime, world, spawnReinforcement);
		return;

	case State::OuterShieldWarning:
		UpdateOuterShieldWarning(deltaTime, world);
		return;

	case State::OuterDestroying:
		UpdateOuterRingDestruction(deltaTime, world);
		return;

	case State::InnerPhase:
		UpdateInnerPhase(deltaTime, world, spawnReinforcement, false);
		return;

	case State::InnerShieldWarning:
		UpdateShield(deltaTime);
		static_cast<void>(world.ConsumePlayerProjectilesInCircle(position, config.innerShieldRadius));
		UpdateInnerPhase(deltaTime, world, spawnReinforcement, true);

		stateElapsed += deltaTime;

		if (stateElapsed >= config.shieldWarningDuration)
		{
			state = State::InnerShield;
			stateElapsed = 0.f;
		}
		return;

	case State::InnerShield:
		UpdateShield(deltaTime);
		HandleShieldImpacts(world, position, config.innerShieldRadius);
		UpdateInnerPhase(deltaTime, world, spawnReinforcement, true);
		stateElapsed += deltaTime;

		if (stateElapsed >= config.innerShieldDuration)
		{
			state = State::InnerPhase;
			stateElapsed = 0.f;
		}
		return;

	case State::InnerDestroying:
		UpdateDiamondDestruction(deltaTime, world);
		return;

	case State::CoreShieldWarning:
	case State::CoreShield:
	case State::CoreExposed:
		UpdateCorePhase(deltaTime, world, spawnReinforcement);
		return;

	case State::CoreDying:
		UpdateCoreDestruction(deltaTime, world);
		return;

	case State::VictorySilence:
		world.DestroyAllBossVictoryTargets();
		stateElapsed += deltaTime;

		if (stateElapsed >= VictorySilenceDuration)
			state = State::VictoryReady;

		return;
	}
}

void BossEncounter::DrawHUD(sf::RenderTarget& target) const
{
	visual.DrawHud(target, *this);
}

bool BossEncounter::IsActive() const noexcept
{
	return state != State::Dormant;
}

bool BossEncounter::IsReady() const noexcept
{
	// "Ready" is everything past the arrival sequence -- shorter (and just as
	// correct) to name the 3 states that AREN'T ready than list the other 14.
	return state != State::Dormant && state != State::Arriving && state != State::ShieldDelay;
}

bool BossEncounter::IsDefeatSequenceActive() const noexcept
{
	return state == State::CoreDying || state == State::VictorySilence || state == State::VictoryReady;
}

bool BossEncounter::IsVictoryReady() const noexcept
{
	return state == State::VictoryReady;
}

void BossEncounter::SetPosition(sf::Vector2f newPosition)
{
	position = newPosition;
}

void BossEncounter::UpdateShield(float deltaTime)
{
	shieldPulse = std::fmod(shieldPulse + deltaTime * 1.35f, 2.f * std::numbers::pi_v<float>);
}

void BossEncounter::UpdateLightning(float deltaTime)
{
	for (Lightning& bolt : lightning)
		bolt.elapsed += deltaTime;

	std::erase_if(lightning, [](const Lightning& bolt) { return bolt.elapsed >= LightningDuration; });
}

void BossEncounter::HandleShieldImpacts(World& world, sf::Vector2f center, float shieldRadius)
{
	const std::vector<World::PlayerProjectileImpact> impacts{ world.ConsumePlayerProjectilesInCircle(center, shieldRadius) };

	for (const World::PlayerProjectileImpact& impact : impacts)
	{
		const sf::Vector2f playerPosition{ world.GetPlayerPosition() };
		lightning.push_back({ impact.position, playerPosition });
		static_cast<void>(world.DamagePlayerFromBoss(ShieldRetaliationDamage, impact.position));
	}
}

void BossEncounter::UpdateOuterRingMechanics(float deltaTime, World& world)
{
	phaseElapsed += deltaTime;

	ringRotationDegrees = std::fmod(ringRotationDegrees + config.outerRingRotationSpeedDegrees * deltaTime, 360.f);

	if (!world.HasPlayer())
		return;

	for (int index = 0; index < config.cannonCount; index++)
	{
		float& nextShot = nextCannonShots[static_cast<std::size_t>(index)];

		while (phaseElapsed >= nextShot)
		{
			const float angleDegrees =
				-90.f + ringRotationDegrees + 360.f * static_cast<float>(index) / static_cast<float>(config.cannonCount);
			// Polar constructor: unit vector (radius 1) pointing along angleDegrees.
			const sf::Vector2f fireDirection(1.f, sf::degrees(angleDegrees));
			const sf::Vector2f cannonPosition{ position + fireDirection * config.cannonOrbitRadius };

			world.Effects().Add({ Rendering::EffectEventType::EnemyMuzzleFlash, cannonPosition, fireDirection, 1.45f });
			world.SpawnSaucerShot(cannonPosition, cannonPosition + fireDirection * 1000.f);
			nextShot += config.cannonFireInterval;
		}
	}
}

void BossEncounter::UpdateOuterShield(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement)
{
	UpdateShield(deltaTime);
	HandleShieldImpacts(world, position, config.outerShieldRadius);
	UpdateOuterRingMechanics(deltaTime, world);

	stateElapsed += deltaTime;
	reinforcementElapsed += deltaTime;

	while (reinforcementElapsed >= nextKamikazeSpawn)
	{
		std::optional<GameplayData::PickupKind> bonus;

		if (nextPhaseOneBonus < config.phaseOneBonusDrops.size() && nextPhaseOneBonus < static_cast<std::size_t>(shieldCycle))
			bonus = config.phaseOneBonusDrops[nextPhaseOneBonus++];

		spawnReinforcement(GameplayData::EnemyKind::Kamikaze, std::nullopt, bonus);
		nextKamikazeSpawn += config.kamikazeSpawnInterval;
	}

	if (shieldCycle >= 2)
	{
		while (reinforcementElapsed >= nextShooterSpawn)
		{
			spawnReinforcement(GameplayData::EnemyKind::Shooter, std::nullopt, std::nullopt);
			nextShooterSpawn += config.shooterSpawnInterval;
		}
	}

	if (stateElapsed >= config.shieldCycleDuration)
	{
		state = State::OuterExposed;
		stateElapsed = 0.f;
	}
}

void BossEncounter::UpdateOuterShieldWarning(float deltaTime, World& world)
{
	UpdateShield(deltaTime);
	UpdateOuterRingMechanics(deltaTime, world);
	static_cast<void>(world.ConsumePlayerProjectilesInCircle(position, config.outerShieldRadius));
	stateElapsed += deltaTime;

	if (stateElapsed >= config.shieldWarningDuration)
	{
		state = State::OuterShield;
		stateElapsed = 0.f;
		reinforcementElapsed = 0.f;
	}
}

void BossEncounter::BeginOuterShield(int cycle)
{
	state = State::OuterShieldWarning;
	stateElapsed = 0.f;
	reinforcementElapsed = 0.f;
	shieldCycle = cycle;
	nextKamikazeSpawn = config.kamikazeSpawnInterval;
	nextShooterSpawn = config.shooterSpawnInterval;
}

void BossEncounter::BeginOuterRingDestruction()
{
	state = State::OuterDestroying;
	stateElapsed = 0.f;
	destructionExplosionElapsed = 0.f;
}

void BossEncounter::UpdateOuterRingDestruction(float deltaTime, World& world)
{
	VibrationProfiles::Apply(world.Haptics(), VibrationProfiles::BossExplosionSustain);

	stateElapsed += deltaTime;
	destructionExplosionElapsed += deltaTime;

	while (destructionExplosionElapsed >= config.outerRingExplosionInterval)
	{
		destructionExplosionElapsed -= config.outerRingExplosionInterval;
		const float radius = Random::Float(config.outerRingInnerRadius, config.outerRingOuterRadius);
		world.Effects().Add({
			Rendering::EffectEventType::ShipExplosion,
			position + VectorMath::RandomDirection() * radius,
			{}, 0.28f });
	}

	if (stateElapsed >= config.outerRingDestructionDuration)
	{
		isOuterRingVisible = false;

		world.Sound().AddSound(Config::Sound::ShipExplosion, 0.68f);
		world.Effects().Add({ Rendering::EffectEventType::ShipExplosion, position, {}, 1.8f });

		state = State::InnerPhase;
		stateElapsed = 0.f;
		nextPortalSpawn = 0.f;
		nextEdgeShooterSpawn = config.edgeShooterSpawnInterval;
	}
}

void BossEncounter::UpdateInnerPhase(float deltaTime, World& world,
	const SpawnReinforcement& spawnReinforcement, bool isShielded)
{
	diamondRotationDegrees = std::fmod(diamondRotationDegrees + config.diamondRotationSpeedDegrees * deltaTime, 360.f);

	std::array<std::optional<sf::Vector2f>, 4> homingTargets;
	for (std::size_t index = 0u; index < portalHealth.size(); index++)
	{
		if (portalHealth[index] > 0)
			homingTargets[index] = GetPortalPosition(index);
	}

	world.BossHomingTargets().Set(homingTargets);

	nextPortalSpawn -= deltaTime;

	if (nextPortalSpawn <= 0.f)
	{
		static constexpr std::array<GameplayData::EnemyKind, 4> portalKinds =
		{
			GameplayData::EnemyKind::Kamikaze,
			GameplayData::EnemyKind::Shooter,
			GameplayData::EnemyKind::Spinner,
			GameplayData::EnemyKind::MissileCarrier
		};

		const int aliveCount =
			static_cast<int>(std::count_if(portalHealth.begin(), portalHealth.end(), [](int value) { return value > 0; }));

		for (std::size_t attempt = 0u; attempt < portalHealth.size(); attempt++)
		{
			const std::size_t index = nextPortalIndex % portalHealth.size();
			nextPortalIndex = (index + 1u) % portalHealth.size();

			if (portalHealth[index] <= 0)
				continue;

			std::optional<GameplayData::PickupKind> bonus;
			const std::size_t unlockedBonusCount = static_cast<std::size_t>(5 - aliveCount);

			if (nextPhaseTwoBonus < config.phaseTwoBonusDrops.size() && nextPhaseTwoBonus < unlockedBonusCount)
				bonus = config.phaseTwoBonusDrops[nextPhaseTwoBonus++];

			spawnReinforcement(portalKinds[index], GetPortalPosition(index), bonus);

			break;
		}

		nextPortalSpawn += config.portalSpawnIntervalPerAlive * static_cast<float>(std::max(1, aliveCount));
	}

	nextEdgeShooterSpawn -= deltaTime;

	if (nextEdgeShooterSpawn <= 0.f)
	{
		spawnReinforcement(GameplayData::EnemyKind::Shooter, std::nullopt, std::nullopt);
		nextEdgeShooterSpawn += config.edgeShooterSpawnInterval;
	}

	if (!isShielded)
		HandlePortalImpacts(world);
}

void BossEncounter::HandlePortalImpacts(World& world)
{
	for (std::size_t index = 0u; index < portalHealth.size(); index++)
	{
		if (portalHealth[index] <= 0)
			continue;

		const auto impacts{ world.ConsumePlayerProjectilesInCircle(GetPortalPosition(index), config.portalCollisionRadius) };
		if (impacts.empty())
			continue;

		for (const World::PlayerProjectileImpact& impact : impacts)
			portalHealth[index] = std::max(0, portalHealth[index] - impact.damage);

		diamondHitFlashRemaining = hitFlashDuration;
		if (portalHealth[index] > 0)
			continue;

		health = std::max(0, health - config.maximumHealth / 10);

		const int aliveCount = static_cast<int>(
			std::count_if(portalHealth.begin(), portalHealth.end(), [](int value) { return value > 0; }));

		nextPortalSpawn = std::min(nextPortalSpawn, config.portalSpawnIntervalPerAlive * static_cast<float>(aliveCount));

		world.Sound().AddSound(Config::Sound::ShipExplosion, 0.92f);
		world.Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPortalPosition(index), {}, 0.72f });
		world.SpawnPickupAt(GameplayData::PickupKind::Health, GetSafePickupPosition(index));

		const bool areAllPortalsDestroyed =
			std::none_of(portalHealth.begin(), portalHealth.end(), [](int value) { return value > 0; });

		if (areAllPortalsDestroyed)
		{
			world.Effects().Add({ Rendering::EffectEventType::BossDestructionShake, position, {}, 8.f, 1000 });
			BeginDiamondDestruction();
		}
		else
		{
			BeginInnerShield();
		}

		return;
	}

	world.ConsumePlayerProjectilesInDiamondFrame(
		position, diamondRotationDegrees, config.diamondFrameVertexRadius, config.diamondFrameHalfThickness);
}

void BossEncounter::BeginInnerShield()
{
	state = State::InnerShieldWarning;
	stateElapsed = 0.f;
}

void BossEncounter::BeginDiamondDestruction()
{
	state = State::InnerDestroying;
	stateElapsed = 0.f;
	destructionExplosionElapsed = 0.f;
}

void BossEncounter::UpdateDiamondDestruction(float deltaTime, World& world)
{
	VibrationProfiles::Apply(world.Haptics(), VibrationProfiles::BossExplosionSustain);

	stateElapsed += deltaTime;
	destructionExplosionElapsed += deltaTime;

	while (destructionExplosionElapsed >= config.diamondExplosionInterval)
	{
		destructionExplosionElapsed -= config.diamondExplosionInterval;
		world.Effects().Add({
			Rendering::EffectEventType::ShipExplosion,
			position + VectorMath::RandomDirection() * config.portalOrbitRadius,
			{}, 0.25f });
	}

	if (stateElapsed >= config.diamondDestructionDuration)
	{
		isDiamondVisible = false;

		world.Sound().AddSound(Config::Sound::ShipExplosion, 0.76f);
		world.Effects().Add({ Rendering::EffectEventType::ShipExplosion, position, {}, 1.45f });

		state = State::CoreShieldWarning;
		stateElapsed = 0.f;
		isCorePhaseInitialized = false;
	}
}

sf::Vector2f BossEncounter::GetSafePickupPosition(std::size_t quadrantIndex) const
{
	static constexpr std::array<sf::Vector2f, 4> QuadrantFractions =
	{
		sf::Vector2f{ 0.25f, 0.25f },
		sf::Vector2f{ 0.75f, 0.25f },
		sf::Vector2f{ 0.75f, 0.75f },
		sf::Vector2f{ 0.25f, 0.75f }
	};

	const sf::Vector2f fraction{ QuadrantFractions[quadrantIndex % QuadrantFractions.size()] };

	return { arenaSize.x * fraction.x, arenaSize.y * fraction.y };
}

sf::Vector2f BossEncounter::GetPortalPosition(std::size_t index) const
{
	static constexpr std::array<sf::Vector2f, 4> PortalLocalOffsets =
	{
		sf::Vector2f{ -4.3f, -148.4f },
		sf::Vector2f{ 144.1f, -9.6f },
		sf::Vector2f{ -4.3f, 148.4f },
		sf::Vector2f{ -152.4f, -9.6f }
	};

	const sf::Vector2f local{ PortalLocalOffsets[index % PortalLocalOffsets.size()] };

	return position + local.rotatedBy(sf::degrees(diamondRotationDegrees));
}

void BossEncounter::UpdateCorePhase(float deltaTime, World& world, const SpawnReinforcement& spawnReinforcement)
{
	if (!isCorePhaseInitialized)
		BeginCoreShieldCycle(0, spawnReinforcement, false);

	coreBeamAngleDegrees =
		std::fmod(coreBeamAngleDegrees + config.coreBeamSpeeds[static_cast<std::size_t>(coreCycle)] * deltaTime, 360.f);
	coreBeamVisualTime += deltaTime;

	const sf::Vector2f beamStart{ position + CoreVisualOffset };
	world.DamagePlayerWithBeam(beamStart, GetCoreBeamEnd(), config.coreBeamWidth, config.coreBeamDamage);

	if (coreCycle >= 1)
	{
		nextCoreAsteroidSpawn -= deltaTime;
		while (nextCoreAsteroidSpawn <= 0.f)
		{
			spawnReinforcement(GameplayData::EnemyKind::BigMeteor, std::nullopt, std::nullopt);
			nextCoreAsteroidSpawn += config.coreAsteroidSpawnInterval;
		}
	}

	if (state == State::CoreShieldWarning)
	{
		UpdateShield(deltaTime);
		static_cast<void>(world.ConsumePlayerProjectilesInCircle(position + CoreVisualOffset, config.coreShieldRadius));
		stateElapsed += deltaTime;

		if (stateElapsed >= config.shieldWarningDuration)
		{
			state = State::CoreShield;
			stateElapsed = 0.f;
		}

		return;
	}

	if (state == State::CoreShield)
	{
		UpdateShield(deltaTime);
		HandleShieldImpacts(world, position + CoreVisualOffset, config.coreShieldRadius);

		if (world.CountActiveLaserTurrets() == 0u)
		{
			state = State::CoreExposed;
			stateElapsed = 0.f;
		}

		return;
	}

	HandleCoreLaser(world, spawnReinforcement);
	HandleCoreImpacts(world, spawnReinforcement);
}

void BossEncounter::BeginCoreShieldCycle(int cycle, const SpawnReinforcement& spawnReinforcement, bool needToShowWarning)
{
	coreCycle = cycle;
	isCorePhaseInitialized = true;
	state = needToShowWarning ? State::CoreShieldWarning : State::CoreShield;
	stateElapsed = 0.f;

	SpawnCoreTurrets(spawnReinforcement);

	if (cycle == 1)
		nextCoreAsteroidSpawn = 0.f;

	if (cycle == 2)
		SpawnCoreStations(spawnReinforcement);
}

void BossEncounter::SpawnCoreTurrets(const SpawnReinforcement& spawnReinforcement)
{
	const float inset = config.coreTurretInset;
	const std::array positions =
	{
		sf::Vector2f{ inset, inset },
		sf::Vector2f{ arenaSize.x - inset, inset },
		sf::Vector2f{ arenaSize.x - inset, arenaSize.y - inset },
		sf::Vector2f{ inset, arenaSize.y - inset }
	};

	for (std::size_t index = 0u; index < positions.size(); index++)
	{
		std::optional<GameplayData::PickupKind> bonus;

		if (index == 0u && coreCycle >= 1 &&
			nextPhaseThreeBonus < config.phaseThreeBonusDrops.size() &&
			nextPhaseThreeBonus < static_cast<std::size_t>(coreCycle))
		{
			bonus = config.phaseThreeBonusDrops[nextPhaseThreeBonus++];
		}

		spawnReinforcement(GameplayData::EnemyKind::LaserTurret, positions[index], bonus);
	}
}

void BossEncounter::SpawnCoreStations(const SpawnReinforcement& spawnReinforcement)
{
	const float inset = config.coreStationInset;
	const std::array positions =
	{
		sf::Vector2f{ arenaSize.x * 0.5f, inset },
		sf::Vector2f{ arenaSize.x - inset, arenaSize.y * 0.5f },
		sf::Vector2f{ arenaSize.x * 0.5f, arenaSize.y - inset },
		sf::Vector2f{ inset, arenaSize.y * 0.5f }
	};

	for (const sf::Vector2f stationPosition : positions)
		spawnReinforcement(GameplayData::EnemyKind::ShooterStation, stationPosition, std::nullopt);
}

void BossEncounter::HandleCoreImpacts(World& world, const SpawnReinforcement& spawnReinforcement)
{
	const auto impacts{ world.ConsumePlayerProjectilesInCircle(position + CoreVisualOffset, config.coreCollisionRadius) };
	int totalDamage = 0;

	for (const World::PlayerProjectileImpact& impact : impacts)
		totalDamage += impact.damage;

	if (totalDamage > 0)
		ApplyCoreDamage(totalDamage, world, spawnReinforcement);
}

void BossEncounter::HandleCoreLaser(World& world, const SpawnReinforcement& spawnReinforcement)
{
	const auto laser = world.ConsumePlayerLaserDamageEvent();
	if (!laser)
		return;

	const sf::Vector2f segment{ laser->end - laser->start };
	const float lengthSquared = segment.lengthSquared();
	if (lengthSquared <= 0.001f)
		return;

	const sf::Vector2f center{ position + CoreVisualOffset };
	const sf::Vector2f relative{ center - laser->start };
	const float projection = std::clamp(relative.dot(segment) / lengthSquared, 0.f, 1.f);
	const sf::Vector2f impactPosition{ laser->start + segment * projection };
	const sf::Vector2f offset{ center - impactPosition };
	const float hitRadius = config.coreCollisionRadius + laser->width * 0.5f;

	if (offset.lengthSquared() > hitRadius * hitRadius)
		return;

	world.RegisterPlayerAttackHit(laser->attackID);
	world.Effects().Add({ Rendering::EffectEventType::ShipHit, impactPosition, segment, 1.15f });

	ApplyCoreDamage(laser->damage, world, spawnReinforcement);
}

void BossEncounter::ApplyCoreDamage(int damage, World& world, const SpawnReinforcement& spawnReinforcement)
{
	const float minimumRatio = coreCycle == 0 ? 0.2f : coreCycle == 1 ? 0.1f : 0.f;
	const int minimumHealth = static_cast<int>(std::ceil(static_cast<float>(config.maximumHealth) * minimumRatio));

	health = std::max(minimumHealth, health - damage);
	coreHitFlashRemaining = hitFlashDuration;

	if (health > minimumHealth)
		return;

	if (coreCycle < 2)
	{
		BeginCoreShieldCycle(coreCycle + 1, spawnReinforcement, true);
	}
	else
	{
		state = State::CoreDying;
		stateElapsed = 0.f;
		coreExplosionElapsed = 0.f;
		victoryCleanupElapsed = 0.f;
		world.Effects().Add({ Rendering::EffectEventType::BossDestructionShake, position + CoreVisualOffset, {}, 12.f, 3500 });
	}
}

void BossEncounter::UpdateCoreDestruction(float deltaTime, World& world)
{
	VibrationProfiles::Apply(world.Haptics(), VibrationProfiles::BossExplosionSustain);

	stateElapsed += deltaTime;
	coreExplosionElapsed += deltaTime;
	victoryCleanupElapsed += deltaTime;

	while (victoryCleanupElapsed >= VictoryCleanupInterval)
	{
		victoryCleanupElapsed -= VictoryCleanupInterval;
		static_cast<void>(world.DestroyNextBossVictoryTarget());
	}

	while (coreExplosionElapsed >= CoreExplosionInterval)
	{
		coreExplosionElapsed -= CoreExplosionInterval;
		const float radius = Random::Float(10.f, config.coreCollisionRadius);
		world.Effects().Add({
			Rendering::EffectEventType::ShipExplosion,
			position + CoreVisualOffset + VectorMath::RandomDirection() * radius,
			{}, Random::Float(0.28f, 0.58f) });
	}

	if (stateElapsed < CoreDeathAnimationDuration)
		return;

	isCoreVisible = false;

	world.DestroyAllBossVictoryTargets();
	world.Sound().AddSound(Config::Sound::ShipExplosion, 0.48f);
	world.Effects().Add({ Rendering::EffectEventType::ShipExplosion, position + CoreVisualOffset, {}, 3.4f });

	state = State::VictorySilence;
	stateElapsed = 0.f;
}

sf::Vector2f BossEncounter::GetCoreBeamEnd() const
{
	const sf::Vector2f start{ position + CoreVisualOffset };

	// Polar constructor: a unit vector (radius 1) pointing along the beam's
	// current angle -- direction.x/y are cos/sin of it, each sweeping [-1, 1],
	// hence the near-zero guards below before dividing by them.
	const sf::Vector2f direction(1.f, sf::degrees(coreBeamAngleDegrees));
	float distance = std::numeric_limits<float>::max();

	if (direction.x > 0.001f)
		distance = std::min(distance, (arenaSize.x - start.x) / direction.x);
	else if (direction.x < -0.001f)
		distance = std::min(distance, -start.x / direction.x);

	if (direction.y > 0.001f)
		distance = std::min(distance, (arenaSize.y - start.y) / direction.y);
	else if (direction.y < -0.001f)
		distance = std::min(distance, -start.y / direction.y);

	return start + direction * (distance + 180.f);
}

float BossEncounter::GetShieldRadius() const noexcept
{
	if (state == State::InnerShieldWarning || state == State::InnerShield)
		return config.innerShieldRadius;

	if (state == State::CoreShieldWarning || state == State::CoreShield)
		return config.coreShieldRadius;

	return config.outerShieldRadius;
}

float BossEncounter::GetEnemyExclusionRadius() const noexcept
{
	if (isOuterRingVisible)
		return config.outerShieldRadius;

	if (isDiamondVisible)
		return config.innerShieldRadius;

	return config.coreShieldRadius;
}

sf::Vector2f BossEncounter::GetCollisionCenter() const noexcept
{
	return !isOuterRingVisible && !isDiamondVisible ? position + CoreVisualOffset : position;
}

bool BossEncounter::IsShieldCollisionActive() const noexcept
{
	return state == State::Arriving || state == State::ShieldDelay ||
		state == State::OuterShieldWarning || state == State::OuterShield ||
		state == State::InnerShieldWarning || state == State::InnerShield ||
		state == State::CoreShieldWarning || state == State::CoreShield;
}

float BossEncounter::GetPlayerExclusionRadius() const noexcept
{
	if (IsShieldCollisionActive())
		return GetShieldRadius();

	if (isOuterRingVisible)
		return config.outerRingOuterRadius;

	if (isDiamondVisible)
		return config.diamondFrameVertexRadius + config.diamondFrameHalfThickness;

	return config.coreCollisionRadius;
}

void BossEncounter::HandleOuterRingImpacts(World& world)
{
	const std::vector<World::PlayerProjectileImpact> impacts{
		world.ConsumePlayerProjectilesInAnnulus(position, config.outerRingInnerRadius, config.outerRingOuterRadius) };

	float minimumHealthRatio = config.outerRingEndHealthRatio;
	if (shieldCycle == 0)
		minimumHealthRatio = config.firstShieldHealthRatio;
	else if (shieldCycle == 1)
		minimumHealthRatio = config.secondShieldHealthRatio;

	const int minimumHealth = static_cast<int>(std::ceil(static_cast<float>(config.maximumHealth) * minimumHealthRatio));

	for (const World::PlayerProjectileImpact& impact : impacts)
		health = std::max(minimumHealth, health - impact.damage);

	if (!impacts.empty())
		ringHitFlashRemaining = hitFlashDuration;
}

void BossEncounter::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	visual.Draw(target, states, *this);
}