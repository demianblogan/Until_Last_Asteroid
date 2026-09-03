#include "BossEncounter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "assets/Assets.h"
#include "gameplay/VibrationProfiles.h"
#include "localization/LocalizationManager.h"
#include "core/world/World.h"
#include "rendering/EnergyShield.h"
#include "utils/ConfigEnums.h"
#include "utils/Pulse.h"
#include "utils/Random.h"

namespace
{
	constexpr sf::Vector2f CoreVisualOffset{ 0.f, -37.f };
	constexpr float OuterShieldRadius{ 355.f };
	constexpr float DiamondShieldRadius{ 225.f };
	constexpr float LightningDuration{ 0.32f };
	constexpr int ShieldRetaliationDamage{ 10 };

	void ConfigureUniformSprite(
		sf::Sprite& sprite,
		float displayedWidth,
		sf::Vector2f origin)
	{
		const sf::Vector2u size{ sprite.getTexture().getSize() };
		sprite.setOrigin(origin);
		const float scale{ displayedWidth / static_cast<float>(size.x) };
		sprite.setScale({ scale, scale });
	}

	float SmoothStep(float value)
	{
		value = std::clamp(value, 0.f, 1.f);
		return value * value * (3.f - 2.f * value);
	}

	// Appends a filled circle (as a triangle fan) into a shared Triangles
	// VertexArray, so a whole cluster of small glow dots/markers can be
	// issued as one draw call instead of one target.draw() per dot.
	void AppendFilledCircle(
		sf::VertexArray& triangles,
		sf::Vector2f center,
		float radius,
		sf::Color color,
		int segments = 12,
		float startAngleRadians = 0.f)
	{
		const float step{ 2.f * std::numbers::pi_v<float> / static_cast<float>(segments) };
		for (int index{ 0 }; index < segments; ++index)
		{
			const float angleA{ startAngleRadians + step * static_cast<float>(index) };
			const float angleB{ startAngleRadians + step * static_cast<float>(index + 1) };
			triangles.append({ center, color });
			triangles.append({ center + sf::Vector2f{
				std::cos(angleA) * radius, std::sin(angleA) * radius }, color });
			triangles.append({ center + sf::Vector2f{
				std::cos(angleB) * radius, std::sin(angleB) * radius }, color });
		}
	}

	void DrawLightningBolt(
		sf::RenderTarget& target,
		const auto& bolt,
		sf::RenderStates states)
	{
		constexpr int SegmentCount{ 18 };
		const sf::Vector2f delta{ bolt.end - bolt.start };
		const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
		if (length <= 0.1f)
			return;
		const sf::Vector2f perpendicular{ -delta.y / length, delta.x / length };
		const float fade{ 1.f - std::clamp(bolt.elapsed / LightningDuration, 0.f, 1.f) };
		const float seed{ bolt.start.x * 0.017f + bolt.start.y * 0.031f };

		sf::RenderStates glowStates{ states };
		glowStates.blendMode = sf::BlendAdd;
		sf::VertexArray glowDots(sf::PrimitiveType::Triangles);
		for (int index{ 0 }; index <= SegmentCount; index += 2)
		{
			const float progress{ static_cast<float>(index) / SegmentCount };
			const float envelope{ std::sin(progress * std::numbers::pi_v<float>) };
			const float jitter{ std::sin(seed + index * 8.73f) * 13.f * envelope };
			const sf::Vector2f point{
				bolt.start + delta * progress + perpendicular * jitter };
			AppendFilledCircle(glowDots, point, 13.f, sf::Color(
				255, 72, 8, static_cast<std::uint8_t>(38.f * fade)));
			AppendFilledCircle(glowDots, point, 5.f, sf::Color(
				255, 190, 72, static_cast<std::uint8_t>(115.f * fade)));
		}
		target.draw(glowDots, glowStates);
		for (int layer{ 0 }; layer < 3; ++layer)
		{
			sf::VertexArray line(sf::PrimitiveType::LineStrip);
			const float layerOffset{ static_cast<float>(layer - 1) * 2.f };
			for (int index{ 0 }; index <= SegmentCount; ++index)
			{
				const float progress{ static_cast<float>(index) / SegmentCount };
				const float envelope{ std::sin(progress * std::numbers::pi_v<float>) };
				const float jitter{ std::sin(seed + index * 8.73f) * 13.f * envelope };
				const sf::Vector2f point{
					bolt.start + delta * progress + perpendicular * (jitter + layerOffset) };
				const auto alpha{ static_cast<std::uint8_t>(
					(95.f - layer * 20.f) * fade) };
				line.append({ point, sf::Color(255, 105, 18, alpha) });
			}
			target.draw(line, glowStates);
		}

		sf::VertexArray coreLine(sf::PrimitiveType::LineStrip);
		for (int index{ 0 }; index <= SegmentCount; ++index)
		{
			const float progress{ static_cast<float>(index) / SegmentCount };
			const float envelope{ std::sin(progress * std::numbers::pi_v<float>) };
			const float jitter{ std::sin(seed + index * 8.73f) * 13.f * envelope };
			coreLine.append({
				bolt.start + delta * progress + perpendicular * jitter,
				sf::Color(255, 225, 145,
					static_cast<std::uint8_t>(255.f * fade)) });
		}
		target.draw(coreLine, glowStates);
	}

	void DrawCoreBeam(
		sf::RenderTarget& target,
		sf::Vector2f start,
		sf::Vector2f end,
		float beamWidth,
		float pulse,
		float animationTime,
		sf::RenderStates states)
	{
		const sf::Vector2f delta{ end - start };
		const float length{ std::sqrt(delta.x * delta.x + delta.y * delta.y) };
		if (length <= 0.1f)
			return;
		const sf::Vector2f direction{ delta / length };
		const sf::Vector2f perpendicular{ -direction.y, direction.x };
		states.blendMode = sf::BlendAdd;
		const float outerHalfWidth{ beamWidth * 2.8f };
		const float glowHalfWidth{ beamWidth * 1.35f };
		const float coreHalfWidth{ beamWidth * 0.5f };
		const auto withAlpha{ [pulse](sf::Color color, float alpha)
		{
			color.a = static_cast<std::uint8_t>(
				std::clamp(alpha * pulse, 0.f, 255.f));
			return color;
		} };
		const std::array offsets{
			-outerHalfWidth, -glowHalfWidth, -coreHalfWidth,
			0.f,
			coreHalfWidth, glowHalfWidth, outerHalfWidth };
		const sf::Color orangeGlow{ 255, 78, 8 };
		const std::array colors{
			withAlpha(orangeGlow, 0.f),
			withAlpha(orangeGlow, 38.f),
			withAlpha(orangeGlow, 150.f),
			withAlpha(sf::Color(255, 238, 196), 255.f),
			withAlpha(orangeGlow, 150.f),
			withAlpha(orangeGlow, 38.f),
			withAlpha(orangeGlow, 0.f) };
		sf::VertexArray beam(sf::PrimitiveType::TriangleStrip);
		beam.resize(offsets.size() * 2u);
		for (std::size_t index{ 0u }; index < offsets.size(); ++index)
		{
			beam[index * 2u] = sf::Vertex{
				start + perpendicular * offsets[index], colors[index] };
			beam[index * 2u + 1u] = sf::Vertex{
				end + perpendicular * offsets[index], colors[index] };
		}
		target.draw(beam, states);

		constexpr int MarkerCount{ 22 };
		sf::VertexArray markers(sf::PrimitiveType::Triangles);
		for (int index{ 0 }; index < MarkerCount; ++index)
		{
			const float base{ static_cast<float>(index) / MarkerCount };
			const float travel{ std::fmod(base + animationTime * 0.7f, 1.f) };
			const float phase{ travel * 4.f * std::numbers::pi_v<float> +
				animationTime * 5.f };
			const float offset{ std::sin(phase) * beamWidth * 0.8f };
			const float depth{ 0.5f + 0.5f * std::cos(phase) };
			const float radius{ 4.f + depth * 5.f };
			const sf::Vector2f markerPosition{ start + direction * (travel * length) +
				perpendicular * offset };
			const float rotationRadians{ (45.f + animationTime * 150.f) *
				(std::numbers::pi_v<float> / 180.f) };
			AppendFilledCircle(markers, markerPosition, radius, sf::Color(255, 174, 65,
				static_cast<std::uint8_t>(135.f + depth * 120.f)), 4, rotationRadians);
		}
		target.draw(markers, states);
	}
}

BossEncounter::BossEncounter(Assets& assets, LocalizationManager& localize, sf::Vector2f logicalSize)
	: localization(localize), startPosition{ logicalSize.x * 0.5f, -380.f }
	, battlePosition{ logicalSize.x * 0.5f, logicalSize.y * 0.43f }
	, arenaSize(logicalSize)
	, outerRing(assets.Textures().Get(Config::Texture::BossOuterRing))
	, diamond(assets.Textures().Get(Config::Texture::BossDiamond))
	, core(assets.Textures().Get(Config::Texture::BossCore))
	, armorLabel(assets.Fonts().Get(localize.GetBoldFont()), "", 22u)
	, hudCenterX(logicalSize.x * 0.5f)
	, hitFlashShader(assets.GetShader(Config::Shader::HitFlash))
	, config(assets.GetGameplayData().GetBoss())
{
	const sf::Vector2u ringSize{ outerRing.getTexture().getSize() };
	ConfigureUniformSprite(outerRing, 650.f,
		{ ringSize.x * 0.5f, ringSize.y * 0.5f });
	const sf::Vector2u diamondSize{ diamond.getTexture().getSize() };
	ConfigureUniformSprite(diamond, 425.f,
		{ diamondSize.x * 0.5f, diamondSize.y * 0.5f });
	// The core sprite's brain silhouette isn't centered in its source image,
	// so the origin is anchored at a specific source pixel rather than the
	// texture's geometric center. That pixel coordinate scales with the
	// texture's own resolution: it was authored against a 1254x1254 source,
	// so scale it the same way for whatever resolution the texture is now.
	const sf::Vector2u coreSize{ core.getTexture().getSize() };
	const float coreOriginScale{ static_cast<float>(coreSize.x) / 1254.f };
	ConfigureUniformSprite(core, 280.f,
		{ 628.f * coreOriginScale, 628.f * coreOriginScale });
	for (sf::CircleShape& mask : destroyedPortalMasks)
	{
		mask.setRadius(config.portalCollisionRadius * 0.86f);
		mask.setOrigin({ mask.getRadius(), mask.getRadius() });
		mask.setFillColor(sf::Color(8, 5, 6, 245));
		mask.setOutlineColor(sf::Color(92, 28, 20, 230));
		mask.setOutlineThickness(3.f);
	}
	hitFlashDuration = assets.GetGameplayData().GetHitFlashDuration();
	armorLabel.setFillColor(sf::Color(255, 238, 232));
	armorLabel.setOutlineColor(sf::Color(80, 0, 0, 230));
	armorLabel.setOutlineThickness(2.f);
	Reset();
}

void BossEncounter::Reset()
{
	state = State::Dormant;
	stateElapsed = 0.f;
	shieldPulse = 0.f;
	health = config.maximumHealth;
	UpdateArmorLabel();
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
	corePhaseInitialized = false;
	outerRingVisible = true;
	diamondVisible = true;
	coreVisible = true;
	nextPortalIndex = 0u;
	nextPhaseOneBonus = 0u;
	nextPhaseTwoBonus = 0u;
	nextPhaseThreeBonus = 0u;
	portalHealth.fill(config.portalHealth);
	nextCannonShots.resize(static_cast<std::size_t>(config.cannonCount));
	for (int index{ 0 }; index < config.cannonCount; ++index)
	{
		nextCannonShots[static_cast<std::size_t>(index)] =
			static_cast<float>(index) * config.cannonStaggerInterval;
	}
	outerRing.setRotation(sf::degrees(0.f));
	diamond.setRotation(sf::degrees(0.f));
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

void BossEncounter::Update(
	float deltaTime,
	World& world,
	const SpawnReinforcement& spawnReinforcement)
{
	ringHitFlashRemaining = std::max(0.f, ringHitFlashRemaining - deltaTime);
	diamondHitFlashRemaining = std::max(0.f, diamondHitFlashRemaining - deltaTime);
	coreHitFlashRemaining = std::max(0.f, coreHitFlashRemaining - deltaTime);
	UpdateLightning(deltaTime);
	if (state == State::Dormant)
		return;
	world.BossHomingTargets().Clear();
	if (!IsDefeatSequenceActive())
	{
		world.KeepPlayerOutsideCircle(
			GetCollisionCenter(), GetPlayerExclusionRadius(), config.contactDamage);
	}
	if (IsReady())
	{
		world.KeepEnemiesOutsideCircle(
			GetCollisionCenter(), GetEnemyExclusionRadius(), config.shieldEnemyClearance);
		world.SetRewardExclusionCircle(
			GetCollisionCenter(), GetEnemyExclusionRadius() + config.shieldEnemyClearance);
	}
	if (state == State::Arriving || state == State::ShieldDelay)
		HandleShieldImpacts(world, position, OuterShieldRadius);
	if (state == State::OuterExposed)
	{
		UpdateOuterRingMechanics(deltaTime, world);
		HandleOuterRingImpacts(world);
		const float healthRatio{
			static_cast<float>(health) / static_cast<float>(config.maximumHealth) };
		if (shieldCycle == 0 && healthRatio <= config.firstShieldHealthRatio)
			BeginOuterShield(1);
		else if (shieldCycle == 1 && healthRatio <= config.secondShieldHealthRatio)
			BeginOuterShield(2);
		else if (shieldCycle == 2 && healthRatio <= config.outerRingEndHealthRatio)
		{
			world.Effects().Add({ Rendering::EffectEventType::BossDestructionShake,
				position, {}, 8.f, 1000 });
			world.SpawnPickupAt(
				GameplayData::PickupKind::Health, GetSafePickupPosition(0u));
			BeginOuterRingDestruction();
		}
		return;
	}
	if (state == State::OuterShield)
	{
		UpdateOuterShield(deltaTime, world, spawnReinforcement);
		return;
	}
	if (state == State::OuterShieldWarning)
	{
		UpdateOuterShieldWarning(deltaTime, world);
		return;
	}
	if (state == State::OuterDestroying)
	{
		UpdateOuterRingDestruction(deltaTime, world);
		return;
	}
	if (state == State::InnerPhase)
	{
		UpdateInnerPhase(deltaTime, world, spawnReinforcement, false);
		return;
	}
	if (state == State::InnerShieldWarning)
	{
		UpdateShield(deltaTime);
		static_cast<void>(world.ConsumePlayerProjectilesInCircle(
			position, DiamondShieldRadius));
		UpdateInnerPhase(deltaTime, world, spawnReinforcement, true);
		stateElapsed += deltaTime;
		if (stateElapsed >= config.shieldWarningDuration)
		{
			state = State::InnerShield;
			stateElapsed = 0.f;
		}
		return;
	}
	if (state == State::InnerShield)
	{
		UpdateShield(deltaTime);
		HandleShieldImpacts(world, position, DiamondShieldRadius);
		UpdateInnerPhase(deltaTime, world, spawnReinforcement, true);
		stateElapsed += deltaTime;
		if (stateElapsed >= config.innerShieldDuration)
		{
			state = State::InnerPhase;
			stateElapsed = 0.f;
		}
		return;
	}
	if (state == State::InnerDestroying)
	{
		UpdateDiamondDestruction(deltaTime, world);
		return;
	}
	if (state == State::CoreShieldWarning || state == State::CoreShield ||
		state == State::CoreExposed)
	{
		UpdateCorePhase(deltaTime, world, spawnReinforcement);
		return;
	}
	if (state == State::CoreDying)
	{
		UpdateCoreDestruction(deltaTime, world);
		return;
	}
	if (state == State::VictorySilence)
	{
		world.DestroyAllBossVictoryTargets();
		stateElapsed += deltaTime;
		if (stateElapsed >= 3.f)
			state = State::VictoryReady;
		return;
	}
	if (state == State::VictoryReady)
		return;

	stateElapsed += deltaTime;
	UpdateShield(deltaTime);
	if (state == State::Arriving)
	{
		const float progress{ SmoothStep(stateElapsed / ArrivalDuration) };
		SetPosition(startPosition + (battlePosition - startPosition) * progress);
		if (stateElapsed >= ArrivalDuration)
		{
			SetPosition(battlePosition);
			state = State::ShieldDelay;
			stateElapsed = 0.f;
		}
	}
	else if (state == State::ShieldDelay && stateElapsed >= ShieldDelayDuration)
	{
		state = State::OuterExposed;
		stateElapsed = 0.f;
		phaseElapsed = 0.f;
	}
}

void BossEncounter::DrawHud(sf::RenderTarget& target) const
{
	if (state == State::Dormant || state == State::Arriving ||
		state == State::ShieldDelay)
		return;

	constexpr float BarWidth{ 920.f };
	constexpr float BarHeight{ 38.f };
	constexpr float Border{ 4.f };
	const float ratio{ std::clamp(
		static_cast<float>(health) / static_cast<float>(config.maximumHealth),
		0.f, 1.f) };

	sf::RectangleShape shadow({ BarWidth + 12.f, BarHeight + 12.f });
	shadow.setPosition({ hudCenterX - (BarWidth + 12.f) * 0.5f, 18.f });
	shadow.setFillColor(sf::Color(0, 0, 0, 175));
	target.draw(shadow);

	sf::RectangleShape frame({ BarWidth, BarHeight });
	frame.setPosition({ hudCenterX - BarWidth * 0.5f, 24.f });
	frame.setFillColor(sf::Color(28, 3, 7, 235));
	frame.setOutlineColor(sf::Color(185, 32, 42, 245));
	frame.setOutlineThickness(2.f);
	target.draw(frame);

	sf::RectangleShape fill({ (BarWidth - Border * 2.f) * ratio,
		BarHeight - Border * 2.f });
	fill.setPosition({ hudCenterX - BarWidth * 0.5f + Border, 24.f + Border });
	fill.setFillColor(sf::Color(205, 18, 35, 235));
	target.draw(fill);
	target.draw(armorLabel);
}

bool BossEncounter::IsActive() const noexcept
{
	return state != State::Dormant;
}

bool BossEncounter::IsReady() const noexcept
{
	return state == State::OuterExposed || state == State::OuterShieldWarning ||
		state == State::OuterShield ||
		state == State::OuterDestroying || state == State::InnerPhase ||
		state == State::InnerShieldWarning || state == State::InnerShield ||
		state == State::InnerDestroying || state == State::CoreShieldWarning ||
		state == State::CoreShield || state == State::CoreExposed ||
		state == State::CoreDying || state == State::VictorySilence ||
		state == State::VictoryReady;
}

bool BossEncounter::IsDefeatSequenceActive() const noexcept
{
	return state == State::CoreDying || state == State::VictorySilence ||
		state == State::VictoryReady;
}

bool BossEncounter::IsVictoryReady() const noexcept
{
	return state == State::VictoryReady;
}

#ifdef _DEBUG
void BossEncounter::DebugAdvancePhase(World& world)
{
	world.ClearProjectiles();
	lightning.clear();
	SetPosition(battlePosition);
	if (state == State::Dormant || state == State::Arriving ||
		state == State::ShieldDelay || state == State::OuterExposed ||
		state == State::OuterShieldWarning || state == State::OuterShield ||
		state == State::OuterDestroying)
	{
		health = static_cast<int>(std::lround(
			static_cast<float>(config.maximumHealth) * config.outerRingEndHealthRatio));
		UpdateArmorLabel();
		outerRingVisible = false;
		diamondVisible = true;
		outerRing.setPosition(position);
		diamond.setPosition(position);
		state = State::InnerPhase;
		stateElapsed = 0.f;
		nextPortalSpawn = 0.f;
		nextEdgeShooterSpawn = config.edgeShooterSpawnInterval;
		return;
	}
	if (state == State::InnerPhase || state == State::InnerShieldWarning ||
		state == State::InnerShield || state == State::InnerDestroying)
	{
		health = config.maximumHealth * 3 / 10;
		UpdateArmorLabel();
		outerRingVisible = false;
		diamondVisible = false;
		diamond.setPosition(position);
		portalHealth.fill(0);
		state = State::CoreShieldWarning;
		stateElapsed = 0.f;
		corePhaseInitialized = false;
	}
}

void BossEncounter::DebugDefeat(World& world)
{
	world.ClearProjectiles();
	lightning.clear();
	SetPosition(battlePosition);
	health = 0;
	UpdateArmorLabel();
	outerRingVisible = false;
	diamondVisible = false;
	coreVisible = true;
	portalHealth.fill(0);
	state = State::CoreDying;
	stateElapsed = 0.f;
	coreExplosionElapsed = 0.f;
	victoryCleanupElapsed = 0.f;
	world.Effects().Add({ Rendering::EffectEventType::BossDestructionShake,
		position + CoreVisualOffset, {}, 12.f, 3500 });
}
#endif

void BossEncounter::SetPosition(sf::Vector2f newPosition)
{
	outerRing.setPosition(newPosition);
	diamond.setPosition(newPosition);
	core.setPosition(newPosition + CoreVisualOffset);
	position = newPosition;
}

void BossEncounter::UpdateShield(float deltaTime)
{
	shieldPulse = std::fmod(
		shieldPulse + deltaTime * 1.35f,
		2.f * std::numbers::pi_v<float>);
}

void BossEncounter::UpdateLightning(float deltaTime)
{
	for (Lightning& bolt : lightning)
		bolt.elapsed += deltaTime;
	std::erase_if(lightning, [](const Lightning& bolt)
	{
		return bolt.elapsed >= LightningDuration;
	});
}

void BossEncounter::HandleShieldImpacts(
	World& world,
	sf::Vector2f center,
	float shieldRadius)
{
	const std::vector<World::PlayerProjectileImpact> impacts{
		world.ConsumePlayerProjectilesInCircle(center, shieldRadius) };
	for (const World::PlayerProjectileImpact& impact : impacts)
	{
		const sf::Vector2f playerPosition{ world.GetPlayerPosition() };
		lightning.push_back({ impact.position, playerPosition });
		static_cast<void>(world.DamagePlayerFromBoss(
			ShieldRetaliationDamage, impact.position));
	}
}

void BossEncounter::UpdateOuterRingMechanics(float deltaTime, World& world)
{
	phaseElapsed += deltaTime;
	ringRotationDegrees = std::fmod(
		ringRotationDegrees + config.outerRingRotationSpeedDegrees * deltaTime,
		360.f);
	outerRing.setRotation(sf::degrees(ringRotationDegrees));

	if (!world.HasPlayer())
		return;
	for (int index{ 0 }; index < config.cannonCount; ++index)
	{
		float& nextShot{ nextCannonShots[static_cast<std::size_t>(index)] };
		while (phaseElapsed >= nextShot)
		{
			const float angleDegrees{ -90.f + ringRotationDegrees +
				360.f * static_cast<float>(index) /
				static_cast<float>(config.cannonCount) };
			const float angleRadians{
				angleDegrees * std::numbers::pi_v<float> / 180.f };
			const sf::Vector2f cannonPosition{ position + sf::Vector2f{
				std::cos(angleRadians) * config.cannonOrbitRadius,
				std::sin(angleRadians) * config.cannonOrbitRadius } };
			const sf::Vector2f fireDirection{
				std::cos(angleRadians), std::sin(angleRadians) };
			world.Effects().Add({
				Rendering::EffectEventType::EnemyMuzzleFlash,
				cannonPosition,
				fireDirection,
				1.45f });
			world.SpawnSaucerShot(
				cannonPosition,
				cannonPosition + fireDirection * 1000.f);
			nextShot += config.cannonFireInterval;
		}
	}
}

void BossEncounter::UpdateOuterShield(
	float deltaTime,
	World& world,
	const SpawnReinforcement& spawnReinforcement)
{
	UpdateShield(deltaTime);
	HandleShieldImpacts(world, position, OuterShieldRadius);
	UpdateOuterRingMechanics(deltaTime, world);
	stateElapsed += deltaTime;
	reinforcementElapsed += deltaTime;

	while (reinforcementElapsed >= nextKamikazeSpawn)
	{
		std::optional<GameplayData::PickupKind> bonus;
		if (nextPhaseOneBonus < config.phaseOneBonusDrops.size() &&
			nextPhaseOneBonus < static_cast<std::size_t>(shieldCycle))
			bonus = config.phaseOneBonusDrops[nextPhaseOneBonus++];
		spawnReinforcement(
			GameplayData::EnemyKind::Kamikaze, std::nullopt, bonus);
		nextKamikazeSpawn += config.kamikazeSpawnInterval;
	}
	if (shieldCycle >= 2)
	{
		while (reinforcementElapsed >= nextShooterSpawn)
		{
			spawnReinforcement(
				GameplayData::EnemyKind::Shooter, std::nullopt, std::nullopt);
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
	static_cast<void>(world.ConsumePlayerProjectilesInCircle(
		position, OuterShieldRadius));
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
	outerRing.setPosition(position);
}

void BossEncounter::UpdateOuterRingDestruction(float deltaTime, World& world)
{
	VibrationProfiles::Apply(world.Haptics(), VibrationProfiles::BossExplosionSustain);

	stateElapsed += deltaTime;
	destructionExplosionElapsed += deltaTime;
	outerRing.setPosition(position + sf::Vector2f{
		Random::Float(-7.f, 7.f), Random::Float(-7.f, 7.f) });

	while (destructionExplosionElapsed >= config.outerRingExplosionInterval)
	{
		destructionExplosionElapsed -= config.outerRingExplosionInterval;
		const float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
		const float radius{ Random::Float(
			config.outerRingInnerRadius, config.outerRingOuterRadius) };
		world.Effects().Add({
			Rendering::EffectEventType::ShipExplosion,
			position + sf::Vector2f{ std::cos(angle) * radius, std::sin(angle) * radius },
			{},
			0.28f });
	}

	if (stateElapsed >= config.outerRingDestructionDuration)
	{
		outerRing.setPosition(position);
		outerRingVisible = false;
		world.Sound().AddSound(Config::Sound::ShipExplosion, 0.68f);
		world.Effects().Add({
			Rendering::EffectEventType::ShipExplosion, position, {}, 1.8f });
		state = State::InnerPhase;
		stateElapsed = 0.f;
		nextPortalSpawn = 0.f;
		nextEdgeShooterSpawn = config.edgeShooterSpawnInterval;
	}
}

void BossEncounter::UpdateInnerPhase(
	float deltaTime,
	World& world,
	const SpawnReinforcement& spawnReinforcement,
	bool shielded)
{
	diamondRotationDegrees = std::fmod(
		diamondRotationDegrees + config.diamondRotationSpeedDegrees * deltaTime,
		360.f);
	diamond.setRotation(sf::degrees(diamondRotationDegrees));
	std::array<std::optional<sf::Vector2f>, 4> homingTargets;
	for (std::size_t index{ 0u }; index < portalHealth.size(); ++index)
	{
		if (portalHealth[index] > 0)
			homingTargets[index] = GetPortalPosition(index);
	}
	world.BossHomingTargets().Set(homingTargets);

	nextPortalSpawn -= deltaTime;
	if (nextPortalSpawn <= 0.f)
	{
		static constexpr std::array<GameplayData::EnemyKind, 4> portalKinds{
				GameplayData::EnemyKind::Kamikaze,
				GameplayData::EnemyKind::Shooter,
				GameplayData::EnemyKind::Spinner,
				GameplayData::EnemyKind::MissileCarrier };
		const int aliveCount{ static_cast<int>(std::count_if(
			portalHealth.begin(), portalHealth.end(), [](int value) { return value > 0; })) };
		for (std::size_t attempt{ 0u }; attempt < portalHealth.size(); ++attempt)
		{
			const std::size_t index{ nextPortalIndex % portalHealth.size() };
			nextPortalIndex = (index + 1u) % portalHealth.size();
			if (portalHealth[index] <= 0)
				continue;
			std::optional<GameplayData::PickupKind> bonus;
			const std::size_t unlockedBonusCount{
				static_cast<std::size_t>(5 - aliveCount) };
			if (nextPhaseTwoBonus < config.phaseTwoBonusDrops.size() &&
				nextPhaseTwoBonus < unlockedBonusCount)
				bonus = config.phaseTwoBonusDrops[nextPhaseTwoBonus++];
			spawnReinforcement(
				portalKinds[index], GetPortalPosition(index), bonus);
			break;
		}
		nextPortalSpawn += config.portalSpawnIntervalPerAlive *
			static_cast<float>(std::max(1, aliveCount));
	}

	nextEdgeShooterSpawn -= deltaTime;
	if (nextEdgeShooterSpawn <= 0.f)
	{
		spawnReinforcement(
			GameplayData::EnemyKind::Shooter, std::nullopt, std::nullopt);
		nextEdgeShooterSpawn += config.edgeShooterSpawnInterval;
	}

	if (!shielded)
		HandlePortalImpacts(world);
}

void BossEncounter::HandlePortalImpacts(World& world)
{
	for (std::size_t index{ 0u }; index < portalHealth.size(); ++index)
	{
		if (portalHealth[index] <= 0)
			continue;
		const auto impacts{ world.ConsumePlayerProjectilesInCircle(
			GetPortalPosition(index), config.portalCollisionRadius) };
		if (impacts.empty())
			continue;
		for (const World::PlayerProjectileImpact& impact : impacts)
			portalHealth[index] = std::max(0, portalHealth[index] - impact.damage);
		diamondHitFlashRemaining = hitFlashDuration;
		if (portalHealth[index] > 0)
			continue;

		health = std::max(0, health - config.maximumHealth / 10);
		UpdateArmorLabel();
		const int aliveCount{ static_cast<int>(std::count_if(
			portalHealth.begin(), portalHealth.end(), [](int value) { return value > 0; })) };
		nextPortalSpawn = std::min(
			nextPortalSpawn,
			config.portalSpawnIntervalPerAlive * static_cast<float>(aliveCount));
		world.Sound().AddSound(Config::Sound::ShipExplosion, 0.92f);
		world.Effects().Add({ Rendering::EffectEventType::ShipExplosion,
			GetPortalPosition(index), {}, 0.72f });
		world.SpawnPickupAt(
			GameplayData::PickupKind::Health, GetSafePickupPosition(index));
		const bool allDestroyed{ std::none_of(
			portalHealth.begin(), portalHealth.end(), [](int value) { return value > 0; }) };
		if (allDestroyed)
		{
			world.Effects().Add({ Rendering::EffectEventType::BossDestructionShake,
				position, {}, 8.f, 1000 });
			BeginDiamondDestruction();
		}
		else
			BeginInnerShield();
		return;
	}

	world.ConsumePlayerProjectilesInDiamondFrame(
		position,
		diamondRotationDegrees,
		config.diamondFrameVertexRadius,
		config.diamondFrameHalfThickness);
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
	diamond.setPosition(position);
}

void BossEncounter::UpdateDiamondDestruction(float deltaTime, World& world)
{
	VibrationProfiles::Apply(world.Haptics(), VibrationProfiles::BossExplosionSustain);

	stateElapsed += deltaTime;
	destructionExplosionElapsed += deltaTime;
	diamond.setPosition(position + sf::Vector2f{
		Random::Float(-6.f, 6.f), Random::Float(-6.f, 6.f) });
	while (destructionExplosionElapsed >= config.diamondExplosionInterval)
	{
		destructionExplosionElapsed -= config.diamondExplosionInterval;
		const float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
		world.Effects().Add({ Rendering::EffectEventType::ShipExplosion,
			position + sf::Vector2f{
				std::cos(angle) * config.portalOrbitRadius,
				std::sin(angle) * config.portalOrbitRadius },
			{}, 0.25f });
	}
	if (stateElapsed >= config.diamondDestructionDuration)
	{
		diamond.setPosition(position);
		diamondVisible = false;
		world.Sound().AddSound(Config::Sound::ShipExplosion, 0.76f);
		world.Effects().Add({ Rendering::EffectEventType::ShipExplosion,
			position, {}, 1.45f });
		state = State::CoreShieldWarning;
		stateElapsed = 0.f;
		corePhaseInitialized = false;
	}
}

sf::Vector2f BossEncounter::GetSafePickupPosition(std::size_t quadrantIndex) const
{
	static constexpr std::array<sf::Vector2f, 4> QuadrantFractions{
		sf::Vector2f{ 0.25f, 0.25f },
		sf::Vector2f{ 0.75f, 0.25f },
		sf::Vector2f{ 0.75f, 0.75f },
		sf::Vector2f{ 0.25f, 0.75f } };
	const sf::Vector2f fraction{
		QuadrantFractions[quadrantIndex % QuadrantFractions.size()] };
	return { arenaSize.x * fraction.x, arenaSize.y * fraction.y };
}

sf::Vector2f BossEncounter::GetPortalPosition(std::size_t index) const
{
	static constexpr std::array<sf::Vector2f, 4> PortalLocalOffsets{
		sf::Vector2f{ -4.3f, -148.4f },
		sf::Vector2f{ 144.1f, -9.6f },
		sf::Vector2f{ -4.3f, 148.4f },
		sf::Vector2f{ -152.4f, -9.6f } };
	const sf::Vector2f local{
		PortalLocalOffsets[index % PortalLocalOffsets.size()] };
	const float angle{ diamondRotationDegrees *
		std::numbers::pi_v<float> / 180.f };
	return position + sf::Vector2f{
		local.x * std::cos(angle) - local.y * std::sin(angle),
		local.x * std::sin(angle) + local.y * std::cos(angle) };
}

void BossEncounter::UpdateCorePhase(
	float deltaTime,
	World& world,
	const SpawnReinforcement& spawnReinforcement)
{
	if (!corePhaseInitialized)
		BeginCoreShieldCycle(0, spawnReinforcement, false);

	coreBeamAngleDegrees = std::fmod(
		coreBeamAngleDegrees + config.coreBeamSpeeds[static_cast<std::size_t>(coreCycle)] *
			deltaTime,
		360.f);
	coreBeamVisualTime += deltaTime;
	const sf::Vector2f beamStart{ position + CoreVisualOffset };
	world.DamagePlayerWithBeam(
		beamStart, GetCoreBeamEnd(), config.coreBeamWidth, config.coreBeamDamage);

	if (coreCycle >= 1)
	{
		nextCoreAsteroidSpawn -= deltaTime;
		while (nextCoreAsteroidSpawn <= 0.f)
		{
			spawnReinforcement(
				GameplayData::EnemyKind::BigMeteor, std::nullopt, std::nullopt);
			nextCoreAsteroidSpawn += config.coreAsteroidSpawnInterval;
		}
	}

	if (state == State::CoreShieldWarning)
	{
		UpdateShield(deltaTime);
		static_cast<void>(world.ConsumePlayerProjectilesInCircle(
			position + CoreVisualOffset, config.coreShieldRadius));
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
		HandleShieldImpacts(
			world, position + CoreVisualOffset, config.coreShieldRadius);
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

void BossEncounter::BeginCoreShieldCycle(
	int cycle,
	const SpawnReinforcement& spawnReinforcement,
	bool useWarning)
{
	coreCycle = cycle;
	corePhaseInitialized = true;
	state = useWarning ? State::CoreShieldWarning : State::CoreShield;
	stateElapsed = 0.f;
	SpawnCoreTurrets(spawnReinforcement);
	if (cycle == 1)
		nextCoreAsteroidSpawn = 0.f;
	if (cycle == 2)
		SpawnCoreStations(spawnReinforcement);
}

void BossEncounter::SpawnCoreTurrets(
	const SpawnReinforcement& spawnReinforcement)
{
	const float inset{ config.coreTurretInset };
	const std::array positions{
		sf::Vector2f{ inset, inset },
		sf::Vector2f{ arenaSize.x - inset, inset },
		sf::Vector2f{ arenaSize.x - inset, arenaSize.y - inset },
		sf::Vector2f{ inset, arenaSize.y - inset } };
	for (std::size_t index{ 0u }; index < positions.size(); ++index)
	{
		std::optional<GameplayData::PickupKind> bonus;
		if (index == 0u && coreCycle >= 1 &&
			nextPhaseThreeBonus < config.phaseThreeBonusDrops.size() &&
			nextPhaseThreeBonus < static_cast<std::size_t>(coreCycle))
		{
			bonus = config.phaseThreeBonusDrops[nextPhaseThreeBonus++];
		}
		spawnReinforcement(
			GameplayData::EnemyKind::LaserTurret, positions[index], bonus);
	}
}

void BossEncounter::SpawnCoreStations(
	const SpawnReinforcement& spawnReinforcement)
{
	const float inset{ config.coreStationInset };
	const std::array positions{
		sf::Vector2f{ arenaSize.x * 0.5f, inset },
		sf::Vector2f{ arenaSize.x - inset, arenaSize.y * 0.5f },
		sf::Vector2f{ arenaSize.x * 0.5f, arenaSize.y - inset },
		sf::Vector2f{ inset, arenaSize.y * 0.5f } };
	for (const sf::Vector2f stationPosition : positions)
	{
		spawnReinforcement(
			GameplayData::EnemyKind::ShooterStation, stationPosition, std::nullopt);
	}
}

void BossEncounter::HandleCoreImpacts(
	World& world,
	const SpawnReinforcement& spawnReinforcement)
{
	const auto impacts{ world.ConsumePlayerProjectilesInCircle(
		position + CoreVisualOffset, config.coreCollisionRadius) };
	int totalDamage{ 0 };
	for (const World::PlayerProjectileImpact& impact : impacts)
		totalDamage += impact.damage;
	if (totalDamage > 0)
		ApplyCoreDamage(totalDamage, world, spawnReinforcement);
}

void BossEncounter::HandleCoreLaser(
	World& world,
	const SpawnReinforcement& spawnReinforcement)
{
	const auto laser{ world.ConsumePlayerLaserDamageEvent() };
	if (!laser)
		return;
	const sf::Vector2f segment{ laser->end - laser->start };
	const float lengthSquared{ segment.x * segment.x + segment.y * segment.y };
	if (lengthSquared <= 0.001f)
		return;
	const sf::Vector2f center{ position + CoreVisualOffset };
	const sf::Vector2f relative{ center - laser->start };
	const float projection{ std::clamp(
		(relative.x * segment.x + relative.y * segment.y) / lengthSquared,
		0.f, 1.f) };
	const sf::Vector2f impactPosition{ laser->start + segment * projection };
	const sf::Vector2f offset{ center - impactPosition };
	const float hitRadius{ config.coreCollisionRadius + laser->width * 0.5f };
	if (offset.x * offset.x + offset.y * offset.y > hitRadius * hitRadius)
		return;
	world.RegisterPlayerAttackHit(laser->attackID);
	world.Effects().Add({ Rendering::EffectEventType::ShipHit,
		impactPosition, segment, 1.15f });
	ApplyCoreDamage(laser->damage, world, spawnReinforcement);
}

void BossEncounter::ApplyCoreDamage(
	int damage,
	World& world,
	const SpawnReinforcement& spawnReinforcement)
{
	const float minimumRatio{ coreCycle == 0 ? 0.2f : coreCycle == 1 ? 0.1f : 0.f };
	const int minimumHealth{ static_cast<int>(std::ceil(
		static_cast<float>(config.maximumHealth) * minimumRatio)) };
	health = std::max(minimumHealth, health - damage);
	coreHitFlashRemaining = hitFlashDuration;
	UpdateArmorLabel();
	if (health > minimumHealth)
		return;
	if (coreCycle < 2)
		BeginCoreShieldCycle(coreCycle + 1, spawnReinforcement, true);
	else
	{
		state = State::CoreDying;
		stateElapsed = 0.f;
		coreExplosionElapsed = 0.f;
		victoryCleanupElapsed = 0.f;
		world.Effects().Add({ Rendering::EffectEventType::BossDestructionShake,
			position + CoreVisualOffset, {}, 12.f, 3500 });
	}
}

void BossEncounter::UpdateCoreDestruction(float deltaTime, World& world)
{
	VibrationProfiles::Apply(world.Haptics(), VibrationProfiles::BossExplosionSustain);

	stateElapsed += deltaTime;
	coreExplosionElapsed += deltaTime;
	victoryCleanupElapsed += deltaTime;
	while (victoryCleanupElapsed >= 0.1f)
	{
		victoryCleanupElapsed -= 0.1f;
		static_cast<void>(world.DestroyNextBossVictoryTarget());
	}
	while (coreExplosionElapsed >= 0.11f)
	{
		coreExplosionElapsed -= 0.11f;
		const float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
		const float radius{ Random::Float(10.f, config.coreCollisionRadius) };
		world.Effects().Add({ Rendering::EffectEventType::ShipExplosion,
			position + CoreVisualOffset + sf::Vector2f{
				std::cos(angle) * radius, std::sin(angle) * radius },
			{}, Random::Float(0.28f, 0.58f) });
	}
	if (stateElapsed < 3.f)
		return;
	coreVisible = false;
	world.DestroyAllBossVictoryTargets();
	world.Sound().AddSound(Config::Sound::ShipExplosion, 0.48f);
	world.Effects().Add({ Rendering::EffectEventType::ShipExplosion,
		position + CoreVisualOffset, {}, 3.4f });
	state = State::VictorySilence;
	stateElapsed = 0.f;
}

sf::Vector2f BossEncounter::GetCoreBeamEnd() const
{
	const sf::Vector2f start{ position + CoreVisualOffset };
	const float angle{ coreBeamAngleDegrees * std::numbers::pi_v<float> / 180.f };
	const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
	float distance{ std::numeric_limits<float>::max() };
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
		return DiamondShieldRadius;
	if (state == State::CoreShieldWarning || state == State::CoreShield)
		return config.coreShieldRadius;
	return OuterShieldRadius;
}

float BossEncounter::GetEnemyExclusionRadius() const noexcept
{
	if (outerRingVisible)
		return OuterShieldRadius;
	if (diamondVisible)
		return DiamondShieldRadius;
	return config.coreShieldRadius;
}

sf::Vector2f BossEncounter::GetCollisionCenter() const noexcept
{
	return !outerRingVisible && !diamondVisible
		? position + CoreVisualOffset
		: position;
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
	if (outerRingVisible)
		return config.outerRingOuterRadius;
	if (diamondVisible)
		return config.diamondFrameVertexRadius + config.diamondFrameHalfThickness;
	return config.coreCollisionRadius;
}

void BossEncounter::HandleOuterRingImpacts(World& world)
{
	const std::vector<World::PlayerProjectileImpact> impacts{
		world.ConsumePlayerProjectilesInAnnulus(
			position,
			config.outerRingInnerRadius,
			config.outerRingOuterRadius) };
	const float minimumHealthRatio{ shieldCycle == 0
		? config.firstShieldHealthRatio
		: shieldCycle == 1
			? config.secondShieldHealthRatio
			: config.outerRingEndHealthRatio };
	const int minimumHealth{ static_cast<int>(std::ceil(
		static_cast<float>(config.maximumHealth) * minimumHealthRatio)) };
	for (const World::PlayerProjectileImpact& impact : impacts)
		health = std::max(minimumHealth, health - impact.damage);
	if (!impacts.empty())
	{
		ringHitFlashRemaining = hitFlashDuration;
		UpdateArmorLabel();
	}
}

void BossEncounter::UpdateArmorLabel()
{
	const int percent{ static_cast<int>(std::ceil(
		100.f * static_cast<float>(health) /
		static_cast<float>(config.maximumHealth))) };
	armorLabel.setString(localization.FormatText("hud.boss_armor", "value", std::to_string(percent)));
	const sf::FloatRect bounds{ armorLabel.getLocalBounds() };
	armorLabel.setOrigin({
		bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y + bounds.size.y * 0.5f });
	armorLabel.setPosition({ hudCenterX, 42.f });
}

void BossEncounter::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	if (!IsActive())
		return;
	sf::RenderStates ringStates{ states };
	if (ringHitFlashRemaining > 0.f && hitFlashDuration > 0.f)
	{
		hitFlashShader.setUniform("source", sf::Shader::CurrentTexture);
		hitFlashShader.setUniform(
			"intensity", ringHitFlashRemaining / hitFlashDuration);
		ringStates.shader = &hitFlashShader;
	}
	if (outerRingVisible)
		target.draw(outerRing, ringStates);
	sf::RenderStates diamondStates{ states };
	if (diamondHitFlashRemaining > 0.f && hitFlashDuration > 0.f)
	{
		hitFlashShader.setUniform("source", sf::Shader::CurrentTexture);
		hitFlashShader.setUniform(
			"intensity", diamondHitFlashRemaining / hitFlashDuration);
		diamondStates.shader = &hitFlashShader;
	}
	if (diamondVisible)
		target.draw(diamond, diamondStates);
	if (diamondVisible)
	{
		for (std::size_t index{ 0u }; index < portalHealth.size(); ++index)
		{
			if (portalHealth[index] > 0)
				continue;
			sf::CircleShape mask{ destroyedPortalMasks[index] };
			mask.setPosition(GetPortalPosition(index));
			target.draw(mask, states);
		}
	}
	sf::RenderStates coreStates{ states };
	if (coreHitFlashRemaining > 0.f && hitFlashDuration > 0.f)
	{
		hitFlashShader.setUniform("source", sf::Shader::CurrentTexture);
		hitFlashShader.setUniform(
			"intensity", coreHitFlashRemaining / hitFlashDuration);
		coreStates.shader = &hitFlashShader;
	}
	if (coreVisible)
		target.draw(core, coreStates);
	const bool coreCombatActive{ state == State::CoreShieldWarning ||
		state == State::CoreShield || state == State::CoreExposed };
	if (coreCombatActive)
	{
		const float pulse{ 0.82f + 0.18f * std::abs(
			std::sin(coreBeamAngleDegrees * 0.12f)) };
		DrawCoreBeam(target, position + CoreVisualOffset, GetCoreBeamEnd(),
			config.coreBeamWidth, pulse, coreBeamVisualTime, states);
	}
	const bool warningState{ state == State::OuterShieldWarning ||
		state == State::InnerShieldWarning || state == State::CoreShieldWarning };
	const bool warningShieldVisible{ warningState &&
		std::fmod(stateElapsed, config.shieldWarningBlinkInterval * 2.f) <
			config.shieldWarningBlinkInterval };
	if (state == State::Arriving || state == State::ShieldDelay ||
		state == State::OuterShield || state == State::InnerShield ||
		state == State::CoreShield ||
		warningShieldVisible)
	{
		const float pulse{ Pulse::Value(shieldPulse, 5.f, 0.72f, 0.28f) };
		Rendering::DrawEnergyShield(
			target, GetCollisionCenter(), GetShieldRadius(), pulse,
			{ 255, 88, 12 }, { 255, 142, 24 }, { 255, 105, 16 }, 34.f, states);
	}
	for (const Lightning& bolt : lightning)
		DrawLightningBolt(target, bolt, states);
}
