#include "GameplayEffects.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <string>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "entities/Player.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "utils/VectorMath.h"

namespace Rendering
{
	namespace
	{
		// These effects' directions should point downward by default when the
		// source vector (e.g. a stationary emitter's velocity) is degenerate.
		constexpr sf::Vector2f DegenerateDirectionFallback{ 0.f, -1.f };

		int ScaledCount(int count, float scale)
		{
			return std::max(1, static_cast<int>(std::round(static_cast<float>(count) * scale)));
		}

		constexpr float EngineEmissionRateHz = 70.f;
		constexpr float EngineEmissionIntervalSeconds = 1.f / EngineEmissionRateHz;
		constexpr int MaximumEngineEmissionCatchUpSteps = 8;

		constexpr int StoneHitMinimumSmokeCount = 1;
		constexpr int StoneHitSmokeCountDivisor = 5;
		// How wide a cone (in radians) each hit's directional particles
		// scatter into around the impact direction -- debris scatters wider
		// than the dust cloud drifting off of it.
		constexpr float StoneHitDebrisSpreadRadians = 1.35f;
		constexpr float StoneHitDustSpreadRadians = 1.5f;
		constexpr float StoneHitDebrisEndSizeFraction = 0.45f;

		constexpr float MetalHitSpreadRadians = 1.55f;

		constexpr int AsteroidExplosionMinimumSmokeCount = 3;
		constexpr int AsteroidExplosionSmokeCountDivisor = 5;
		constexpr float AsteroidExplosionDebrisEndSizeFraction = 0.55f;
		constexpr float AsteroidLargeSizeThreshold = 0.8f;

		constexpr int ShipExplosionMinimumSmokeCount = 4;
		constexpr int ShipExplosionSmokeCountDivisor = 6;
		// The spark/debris split below fires every other burst particle as a
		// spark (short-lived, small) instead of debris (full lifetime); this
		// is that spark's lifetime as a fraction of the debris burst's own,
		// and the fraction of the debris burst's velocity a debris chunk
		// (versus a spark) actually flies out at.
		constexpr float ShipExplosionSparkLifetimeFraction = 0.65f;
		constexpr float ShipExplosionDebrisSpeedFraction = 0.72f;

		constexpr float StationExplosionDebrisScaleMultiplier = 1.45f;
		constexpr float StationExplosionDebrisMinimumSpeedFraction = 0.9f;
		constexpr float StationExplosionDebrisMaximumSpeedFraction = 1.3f;

		constexpr float ScorePopupDuration = 0.5f;
		constexpr float ScorePopupRiseDistance = 56.f;

		sf::Color ToColor(const std::array<int, 4>& channels)
		{
			return sf::Color(
				static_cast<std::uint8_t>(channels[0]), static_cast<std::uint8_t>(channels[1]),
				static_cast<std::uint8_t>(channels[2]), static_cast<std::uint8_t>(channels[3]));
		}
	}

	GameplayEffects::ScorePopup::ScorePopup(const sf::Font& font, int points, sf::Vector2f position)
		: text(font, "+" + std::to_string(points), 30u)
	{
		text.setFillColor(sf::Color(225, 252, 255));
		text.setOutlineColor(sf::Color(20, 175, 255, 230));
		text.setOutlineThickness(2.f);

		const sf::FloatRect bounds{ text.getLocalBounds() };

		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
		text.setPosition(position);
	}

	GameplayEffects::GameplayEffects(const GameplayData::EffectsConfig& config, Assets& assets)
		: config(config)
		, scorePopupFont(assets.Fonts().Get(Config::Font::MenuSemibold))
	{}

	void GameplayEffects::Update(float deltaTime, World& world, bool isScreenShakeEnabled, bool needToShowScorePopups)
	{
		if (!needToShowScorePopups)
			scorePopups.clear();

		isShakeEnabled = isScreenShakeEnabled;
		if (!isShakeEnabled)
		{
			shakeRemaining = 0.f;
			shakeDuration = 0.f;
			shakeAmplitude = 0.f;
			cameraOffset = {};
		}

		projectileGlowParticles.Clear();

		Player* player = world.GetPlayer();
		const auto playerState = player ? player->GetEffectState() : std::nullopt;

		if (!playerState || !playerState->isThrusting)
		{
			engineEmissionAccumulator = 0.f;
		}
		else
		{
			engineEmissionAccumulator += deltaTime;
			int emittedSteps = 0;
			while (engineEmissionAccumulator >= EngineEmissionIntervalSeconds &&
				emittedSteps < MaximumEngineEmissionCatchUpSteps)
			{
				EmitPlayerEngineParticles(world);
				engineEmissionAccumulator -= EngineEmissionIntervalSeconds;
				emittedSteps++;
			}

			engineEmissionAccumulator = std::min(engineEmissionAccumulator, EngineEmissionIntervalSeconds);
		}

		// A dispatch table (EffectEventType -> std::function/member-pointer)
		// was considered here instead of this switch, but doesn't actually
		// come out simpler: half these cases need extra per-event data the
		// others don't (PlayerHit also drives the damage vignette and a
		// camera shake; BossDestructionShake and ScorePopup don't call an
		// Emit* at all), so a table's entries would need to be lambdas
		// anyway -- no less code than the switch, just harder to read -- and
		// a switch with no default: gets a compiler warning for free if a
		// new EffectEventType is ever added and forgotten here, which a
		// table can't offer.
		for (const EffectEvent& event : world.Effects().Get())
		{
			switch (event.type)
			{
			case EffectEventType::PlayerProjectileGlow:
				EmitProjectileGlow(true, event.position);
				break;
			case EffectEventType::PlayerHomingProjectileGlow:
				EmitProjectileGlow(true, event.position, true);
				break;
			case EffectEventType::PlayerTripleProjectileGlow:
				EmitProjectileGlow(true, event.position, false, true);
				break;
			case EffectEventType::EnemyProjectileGlow:
				EmitProjectileGlow(false, event.position);
				break;
			case EffectEventType::MissileSmoke:
				EmitMissileSmoke(event.position, event.direction);
				break;
			case EffectEventType::EnemyEngine:
				EmitEnemyEngine(event.position, event.direction);
				break;
			case EffectEventType::StationWelding:
				EmitStationWelding(event.position, event.scale);
				break;
			case EffectEventType::StationChainExplosion:
				EmitStationChainExplosion(event.position, event.scale);
				break;
			case EffectEventType::PlayerMuzzleFlash:
				EmitMuzzleFlash(true, event.position, event.direction, event.scale);
				break;
			case EffectEventType::EnemyMuzzleFlash:
				EmitMuzzleFlash(false, event.position, event.direction, event.scale);
				break;
			case EffectEventType::AsteroidHit:
				EmitStoneHit(event.position, event.direction, event.scale);
				break;
			case EffectEventType::ShipHit:
				EmitMetalHit(event.position, event.direction, event.scale);
				break;
			case EffectEventType::PlayerHit:
				HandlePlayerHit(event.position, event.direction, event.scale);
				break;
			case EffectEventType::AsteroidExplosion:
				EmitAsteroidExplosion(event.position, event.scale);
				break;
			case EffectEventType::ShipExplosion:
				EmitShipExplosion(event.position, event.scale);
				break;
			case EffectEventType::StationExplosion:
				EmitStationExplosion(event.position, event.scale);
				break;
			case EffectEventType::PlayerTeleport:
				EmitPlayerTeleport(event.position, event.scale);
				break;
			case EffectEventType::BossDestructionShake:
				StartCameraShake({ static_cast<float>(event.value) / 1000.f, event.scale }, 1.f);
				break;
			case EffectEventType::ScorePopup:
				if (needToShowScorePopups)
					EmitScorePopup(event.position, event.value);
				break;
			}
		}

		world.Effects().Clear();

		engineParticles.Update(deltaTime);
		// Glow particles are cleared and fully re-emitted from scratch this
		// same frame above (see world.Effects().Get()'s loop) rather than
		// aging across frames, so there's nothing here for Update() to
		// simulate -- just rebuild the vertex data for what was just spawned.
		projectileGlowParticles.RefreshVertices();
		weaponParticles.Update(deltaTime);
		impactParticles.Update(deltaTime);
		smokeParticles.Update(deltaTime);
		debrisParticles.Update(deltaTime);
		shockwaveParticles.Update(deltaTime);

		UpdateScorePopups(deltaTime);
		UpdateCameraShake(deltaTime);
		UpdatePostProcess(deltaTime);
	}

	void GameplayEffects::DrawBehindEntities(sf::RenderTarget& target, sf::RenderStates states) const
	{
		target.draw(smokeParticles, states);
		target.draw(engineParticles, states);
		target.draw(projectileGlowParticles, states);
		target.draw(shockwaveParticles, states);
	}

	void GameplayEffects::DrawAboveEntities(sf::RenderTarget& target, sf::RenderStates states) const
	{
		target.draw(weaponParticles, states);
		target.draw(debrisParticles, states);
		target.draw(impactParticles, states);

		for (const ScorePopup& popup : scorePopups)
			target.draw(popup.text, states);
	}

	void GameplayEffects::Clear()
	{
		engineEmissionAccumulator = 0.f;
		shakeRemaining = 0.f;
		shakeDuration = 0.f;
		shakeAmplitude = 0.f;
		cameraOffset = {};
		postProcessState = {};
		shockwaves.clear();
		engineParticles.Clear();
		projectileGlowParticles.Clear();
		weaponParticles.Clear();
		impactParticles.Clear();
		smokeParticles.Clear();
		debrisParticles.Clear();
		shockwaveParticles.Clear();
		scorePopups.clear();
	}

	std::size_t GameplayEffects::GetParticleCount() const noexcept
	{
		return engineParticles.GetParticleCount() + projectileGlowParticles.GetParticleCount() +
			weaponParticles.GetParticleCount() + impactParticles.GetParticleCount() +
			smokeParticles.GetParticleCount() + debrisParticles.GetParticleCount() +
			shockwaveParticles.GetParticleCount();
	}

	sf::Vector2f GameplayEffects::GetCameraOffset() const noexcept
	{
		return cameraOffset;
	}

	const GameplayEffects::PostProcessState& GameplayEffects::GetPostProcessState() const noexcept
	{
		return postProcessState;
	}

	const GameplayData::ParticlePresetConfig& GameplayEffects::Preset(const char* name) const
	{
		const auto it{ config.particlePresets.find(name) };
		if (it == config.particlePresets.end())
			throw std::runtime_error(std::string("Unknown particle preset '") + name + "'");

		return it->second;
	}

	ParticleSpawn GameplayEffects::MakeSpawn(
		const GameplayData::ParticlePresetConfig& preset,
		const sf::Vector2f& position,
		const sf::Vector2f& velocity,
		float rotation,
		float startSizeScale,
		float endSizeScale) const
	{
		return ParticleSpawn
		{
			.position = position,
			.velocity = velocity,
			.lifetime = Random::Float(preset.lifetime.minimum, preset.lifetime.maximum),
			.startSize = Random::Float(preset.startSize.minimum, preset.startSize.maximum) * startSizeScale,
			.endSize = Random::Float(preset.endSize.minimum, preset.endSize.maximum) * endSizeScale,
			.startColor = ToColor(preset.startColor),
			.endColor = ToColor(preset.endColor),
			.rotation = rotation,
			.angularVelocity = Random::Float(preset.angularVelocity.minimum, preset.angularVelocity.maximum),
			.drag = preset.drag,
			.aspectRatio = Random::Float(preset.aspectRatio.minimum, preset.aspectRatio.maximum)
		};
	}

	void GameplayEffects::EmitPlayerEngineParticles(const World& world)
	{
		Player* player = world.GetPlayer();
		const auto playerState = player ? player->GetEffectState() : std::nullopt;

		if (!playerState)
			return;

		const sf::Vector2f perpendicular{ -playerState->exhaustDirection.y,	playerState->exhaustDirection.x };
		const auto& preset{ Preset("engine_player_thruster") };

		// The exhaust plume drags along a fraction of the ship's own motion
		// on top of its outward blast, so it visibly trails behind a turning
		// or accelerating ship instead of firing in a fixed direction.
		constexpr float ShipVelocityInheritance = 0.18f;
		constexpr float PositionJitter = 1.5f;

		for (const sf::Vector2f& emitterPosition : playerState->enginePositions)
		{
			const float exhaustSpeed{ Random::Float(preset.speed.minimum, preset.speed.maximum) };
			const float sidewaysJitter{ Random::Float(preset.lateralJitter.minimum, preset.lateralJitter.maximum) };

			const sf::Vector2f position{ emitterPosition + perpendicular * Random::Float(-PositionJitter, PositionJitter) };
			const sf::Vector2f velocity{
				playerState->velocity * ShipVelocityInheritance +
				playerState->exhaustDirection * exhaustSpeed +
				perpendicular * sidewaysJitter };

			engineParticles.Emit(MakeSpawn(preset, position, velocity));
		}
	}

	void GameplayEffects::EmitProjectileGlow(bool isPlayerProjectile, const sf::Vector2f& position,
		bool isHomingProjectile, bool isTripleShot)
	{
		const char* presetName;
		if (isTripleShot)
			presetName = "projectile_glow_triple";
		else if (isHomingProjectile)
			presetName = "projectile_glow_homing";
		else if (isPlayerProjectile)
			presetName = "projectile_glow_player";
		else
			presetName = "projectile_glow_enemy";

		projectileGlowParticles.Emit(MakeSpawn(Preset(presetName), position));
	}

	void GameplayEffects::EmitMissileSmoke(const sf::Vector2f& position, const sf::Vector2f& direction)
	{
		const sf::Vector2f normalized{ VectorMath::Normalize(direction, DegenerateDirectionFallback) };
		const sf::Vector2f perpendicular{ -normalized.y, normalized.x };
		const sf::Vector2f exhaustPosition{ position - normalized * 20.f };

		const auto& trailPreset{ Preset("missile_smoke_trail") };
		const sf::Vector2f trailPosition{
			exhaustPosition + perpendicular * Random::Float(trailPreset.spawnOffset.minimum, trailPreset.spawnOffset.maximum) };
		const sf::Vector2f trailVelocity{
			-normalized * Random::Float(trailPreset.speed.minimum, trailPreset.speed.maximum) +
			perpendicular * Random::Float(trailPreset.lateralJitter.minimum, trailPreset.lateralJitter.maximum) };

		smokeParticles.Emit(MakeSpawn(trailPreset, trailPosition, trailVelocity));

		const auto& flarePreset{ Preset("missile_smoke_flare") };
		const sf::Vector2f flareVelocity{ -normalized * Random::Float(flarePreset.speed.minimum, flarePreset.speed.maximum) };

		weaponParticles.Emit(MakeSpawn(flarePreset, exhaustPosition, flareVelocity));
	}

	void GameplayEffects::EmitEnemyEngine(const sf::Vector2f& position, const sf::Vector2f& direction)
	{
		const sf::Vector2f normalized{ VectorMath::Normalize(direction, DegenerateDirectionFallback) };
		const sf::Vector2f perpendicular{ -normalized.y, normalized.x };
		const auto& preset{ Preset("enemy_engine_trail") };

		const sf::Vector2f spawnPosition{
			position + perpendicular * Random::Float(preset.spawnOffset.minimum, preset.spawnOffset.maximum) };
		const sf::Vector2f velocity{
			normalized * Random::Float(preset.speed.minimum, preset.speed.maximum) +
			perpendicular * Random::Float(preset.lateralJitter.minimum, preset.lateralJitter.maximum) };

		engineParticles.Emit(MakeSpawn(preset, spawnPosition, velocity));
	}

	void GameplayEffects::EmitStationWelding(const sf::Vector2f& position, float scale)
	{
		const sf::Vector2f source =
		{
			position.x + Random::Float(-26.f, 26.f) * scale,
			position.y + Random::Float(-26.f, 26.f) * scale
		};

		const auto& glowPreset{ Preset("station_welding_glow") };
		impactParticles.Emit(MakeSpawn(glowPreset, source, {}, 0.f, scale, 1.f));

		const auto& sparkPreset{ Preset("station_welding_spark") };
		for (int index = 0; index < sparkPreset.count; index++)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const float angle = std::atan2(direction.y, direction.x);
			const sf::Vector2f velocity{ direction * Random::Float(sparkPreset.speed.minimum, sparkPreset.speed.maximum) };

			impactParticles.Emit(MakeSpawn(sparkPreset, source, velocity, angle, scale, 1.f));
		}
	}

	void GameplayEffects::EmitStationChainExplosion(const sf::Vector2f& position, float scale)
	{
		impactParticles.Emit(MakeSpawn(Preset("station_chain_explosion_core"), position, {}, 0.f, scale, scale));
		impactParticles.Emit(MakeSpawn(Preset("station_chain_explosion_glow"), position, {}, 0.f, scale, scale));

		const auto& debrisPreset{ Preset("station_chain_explosion_debris") };
		for (int index = 0; index < debrisPreset.count; index++)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const float angle = std::atan2(direction.y, direction.x);
			const sf::Vector2f velocity{ direction * Random::Float(debrisPreset.speed.minimum, debrisPreset.speed.maximum) };

			impactParticles.Emit(MakeSpawn(debrisPreset, position, velocity, angle, scale, 1.f));
		}

		const auto& smokePreset{ Preset("station_chain_explosion_smoke") };
		for (int index = 0; index < smokePreset.count; index++)
		{
			const sf::Vector2f spawnPosition{
				position + VectorMath::RandomDirection() * Random::Float(smokePreset.spawnOffset.minimum, smokePreset.spawnOffset.maximum) };
			const sf::Vector2f velocity{
				VectorMath::RandomDirection() * Random::Float(smokePreset.speed.minimum, smokePreset.speed.maximum) };

			smokeParticles.Emit(MakeSpawn(smokePreset, spawnPosition, velocity, 0.f, scale, scale));
		}
	}

	void GameplayEffects::EmitPlayerTeleport(const sf::Vector2f& position, float scale)
	{
		shockwaveParticles.Emit(MakeSpawn(Preset("player_teleport_shockwave"), position, {}, 0.f, scale, scale));
		impactParticles.Emit(MakeSpawn(Preset("player_teleport_flash"), position, {}, 0.f, scale, 1.f));

		const auto& sparkPreset{ Preset("player_teleport_spark") };
		for (int index = 0; index < sparkPreset.count; ++index)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const float angle = std::atan2(direction.y, direction.x);
			const sf::Vector2f spawnPosition{
				position + direction * Random::Float(sparkPreset.spawnOffset.minimum, sparkPreset.spawnOffset.maximum) * scale };
			const sf::Vector2f velocity{ direction * Random::Float(sparkPreset.speed.minimum, sparkPreset.speed.maximum) };

			impactParticles.Emit(MakeSpawn(sparkPreset, spawnPosition, velocity, angle, scale, 1.f));
		}
	}

	void GameplayEffects::EmitMuzzleFlash(bool isPlayerProjectile, const sf::Vector2f& position,
		const sf::Vector2f& direction, float scale)
	{
		const auto& corePreset{ Preset(isPlayerProjectile ? "muzzle_core_player" : "muzzle_core_enemy") };
		const sf::Vector2f coreVelocity{ direction * Random::Float(corePreset.speed.minimum, corePreset.speed.maximum) };
		weaponParticles.Emit(MakeSpawn(corePreset, position + direction * 3.f, coreVelocity, 0.f, scale, scale));

		const auto& haloPreset{ Preset(isPlayerProjectile ? "muzzle_halo_player" : "muzzle_halo_enemy") };
		const sf::Vector2f haloVelocity{ direction * Random::Float(haloPreset.speed.minimum, haloPreset.speed.maximum) };
		weaponParticles.Emit(MakeSpawn(haloPreset, position + direction * 7.f, haloVelocity, 0.f, scale, scale));

		const sf::Vector2f perpendicular{ -direction.y, direction.x };
		const auto& sparkPreset{ Preset(isPlayerProjectile ? "muzzle_spark_player" : "muzzle_spark_enemy") };
		for (int i = 0; i < ScaledCount(sparkPreset.count, scale); i++)
		{
			const sf::Vector2f velocity{
				direction * Random::Float(sparkPreset.speed.minimum, sparkPreset.speed.maximum) +
				perpendicular * Random::Float(sparkPreset.lateralJitter.minimum, sparkPreset.lateralJitter.maximum) };

			weaponParticles.Emit(MakeSpawn(sparkPreset, position, velocity, 0.f, scale, scale));
		}
	}

	void GameplayEffects::EmitStoneHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale)
	{
		const auto& burst = config.stoneHit;
		impactParticles.Emit(MakeSpawn(Preset("stone_hit_flash"), position, {}, 0.f, scale, 1.f));

		const auto& debrisPreset{ Preset("stone_hit_debris") };
		for (int i = 0; i < ScaledCount(burst.count, scale); i++)
		{
			const sf::Vector2f particleDirection{ VectorMath::RandomDirectionAround(direction, StoneHitDebrisSpreadRadians, DegenerateDirectionFallback) };
			const float size = Random::Float(burst.minimumSize, burst.maximumSize) * scale;
			const float lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime);
			const sf::Vector2f velocity{ particleDirection * Random::Float(burst.minimumSpeed, burst.maximumSpeed) };
			const float rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);

			ParticleSpawn spawn{ MakeSpawn(debrisPreset, position, velocity, rotation) };
			spawn.lifetime = lifetime;
			spawn.startSize = size;
			spawn.endSize = size * StoneHitDebrisEndSizeFraction;

			debrisParticles.Emit(spawn);
		}

		const auto& dustPreset{ Preset("stone_hit_dust") };
		for (int i = 0; i < std::max(StoneHitMinimumSmokeCount, ScaledCount(burst.count, scale) / StoneHitSmokeCountDivisor); i++)
		{
			const sf::Vector2f spawnPosition{
				position + VectorMath::RandomDirection() * Random::Float(dustPreset.spawnOffset.minimum, dustPreset.spawnOffset.maximum) };
			const sf::Vector2f velocity{
				VectorMath::RandomDirectionAround(direction, StoneHitDustSpreadRadians, DegenerateDirectionFallback) *
				Random::Float(dustPreset.speed.minimum, dustPreset.speed.maximum) };

			smokeParticles.Emit(MakeSpawn(dustPreset, spawnPosition, velocity, 0.f, scale, scale));
		}
	}

	void GameplayEffects::EmitMetalHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale)
	{
		const auto& burst = config.metalHit;
		impactParticles.Emit(MakeSpawn(Preset("metal_hit_flash"), position, {}, 0.f, scale, 1.f));

		const auto& sparkPreset{ Preset("metal_hit_spark") };
		for (int i = 0; i < ScaledCount(burst.count, scale); i++)
		{
			const sf::Vector2f sparkDirection{ 
				VectorMath::RandomDirectionAround(direction, MetalHitSpreadRadians, DegenerateDirectionFallback) };
			const float speed = Random::Float(burst.minimumSpeed, burst.maximumSpeed);
			const float angle = std::atan2(sparkDirection.y, sparkDirection.x);
			const float lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime);
			const float size = Random::Float(burst.minimumSize, burst.maximumSize) * scale;

			ParticleSpawn spawn{ MakeSpawn(sparkPreset, position, sparkDirection * speed, angle) };
			spawn.lifetime = lifetime;
			spawn.startSize = size;

			impactParticles.Emit(spawn);
		}
	}

	void GameplayEffects::HandlePlayerHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale)
	{
		EmitMetalHit(position, direction, scale);
		postProcessState.damageVignette = 1.f;
		StartCameraShake(config.damageShake, scale);
	}

	void GameplayEffects::EmitAsteroidExplosion(const sf::Vector2f& position, float scale)
	{
		const bool isLarge = scale >= AsteroidLargeSizeThreshold;
		const auto& burst = isLarge ? config.largeAsteroidExplosion : config.smallAsteroidExplosion;
		impactParticles.Emit(MakeSpawn(Preset("asteroid_explosion_flash"), position, {}, 0.f, scale, 1.f));
		impactParticles.Emit(MakeSpawn(Preset("asteroid_explosion_glow"), position, {}, 0.f, scale, scale));

		const auto& debrisPreset{ Preset("asteroid_explosion_debris") };
		for (int i = 0; i < ScaledCount(burst.count, scale); i++)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const float size = Random::Float(burst.minimumSize, burst.maximumSize);
			const sf::Vector2f spawnPosition{ position + direction * Random::Float(0.f, 10.f * scale) };
			const sf::Vector2f velocity{ direction * Random::Float(burst.minimumSpeed, burst.maximumSpeed) };
			const float lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime);
			const float rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);

			ParticleSpawn spawn{ MakeSpawn(debrisPreset, spawnPosition, velocity, rotation) };
			spawn.lifetime = lifetime;
			spawn.startSize = size;
			spawn.endSize = size * AsteroidExplosionDebrisEndSizeFraction;

			debrisParticles.Emit(spawn);
		}

		const auto& smokePreset{ Preset("asteroid_explosion_smoke") };
		const int smokeCount = std::max(AsteroidExplosionMinimumSmokeCount, ScaledCount(burst.count, scale) / AsteroidExplosionSmokeCountDivisor);
		for (int i = 0; i < smokeCount; i++)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const sf::Vector2f spawnPosition{
				position + direction * Random::Float(smokePreset.spawnOffset.minimum, smokePreset.spawnOffset.maximum) * scale };
			const sf::Vector2f velocity{ direction * Random::Float(smokePreset.speed.minimum, smokePreset.speed.maximum) };
			const float rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);

			smokeParticles.Emit(MakeSpawn(smokePreset, spawnPosition, velocity, rotation, scale, scale));
		}

		if (isLarge)
		{
			shockwaveParticles.Emit(MakeSpawn(Preset("asteroid_explosion_shockwave"), position, {}, 0.f, 1.f, scale));

			StartCameraShake(config.largeExplosionShake, 0.8f * scale);
			StartShockwave(position, scale);
		}
	}

	void GameplayEffects::EmitShipExplosion(const sf::Vector2f& position, float scale)
	{
		const auto& burst = config.shipExplosion;
		impactParticles.Emit(MakeSpawn(Preset("ship_explosion_flash"), position, {}, 0.f, scale, 1.f));
		impactParticles.Emit(MakeSpawn(Preset("ship_explosion_glow"), position, {}, 0.f, scale, scale));

		const auto& sparkPreset{ Preset("ship_explosion_spark") };
		const auto& debrisPreset{ Preset("ship_explosion_debris") };
		for (int i = 0; i < ScaledCount(burst.count, scale); ++i)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const float speed = Random::Float(burst.minimumSpeed, burst.maximumSpeed);
			if (i % 2 == 0)
			{
				const float angle = std::atan2(direction.y, direction.x);
				const float lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime) * ShipExplosionSparkLifetimeFraction;

				ParticleSpawn spawn{ MakeSpawn(sparkPreset, position, direction * speed, angle) };
				spawn.lifetime = lifetime;

				impactParticles.Emit(spawn);
			}
			else
			{
				const float size = Random::Float(burst.minimumSize, burst.maximumSize);
				const float lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime);
				const float rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);

				ParticleSpawn spawn{ MakeSpawn(debrisPreset, position, direction * speed * ShipExplosionDebrisSpeedFraction, rotation) };
				spawn.lifetime = lifetime;
				spawn.startSize = size;
				spawn.endSize = size * 0.5f;

				debrisParticles.Emit(spawn);
			}
		}

		const auto& smokePreset{ Preset("ship_explosion_smoke") };
		const int smokeCount = std::max(ShipExplosionMinimumSmokeCount, ScaledCount(burst.count, scale) / ShipExplosionSmokeCountDivisor);
		for (int i = 0; i < smokeCount; i++)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const sf::Vector2f spawnPosition{
				position + direction * Random::Float(smokePreset.spawnOffset.minimum, smokePreset.spawnOffset.maximum) * scale };
			const sf::Vector2f velocity{ direction * Random::Float(smokePreset.speed.minimum, smokePreset.speed.maximum) };
			const float rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);

			smokeParticles.Emit(MakeSpawn(smokePreset, spawnPosition, velocity, rotation, scale, scale));
		}

		shockwaveParticles.Emit(MakeSpawn(Preset("ship_explosion_shockwave"), position, {}, 0.f, 1.f, scale));

		StartCameraShake(config.largeExplosionShake, scale);
		StartShockwave(position, scale);
	}

	void GameplayEffects::EmitStationExplosion(const sf::Vector2f& position, float scale)
	{
		const auto& burst = config.shipExplosion;
		impactParticles.Emit(MakeSpawn(Preset("station_explosion_flash"), position, {}, 0.f, scale, 1.f));
		impactParticles.Emit(MakeSpawn(Preset("station_explosion_glow"), position, {}, 0.f, scale, scale));

		const auto& debrisPreset{ Preset("station_explosion_debris") };
		for (int index = 0; index < ScaledCount(burst.count, scale * StationExplosionDebrisScaleMultiplier); index++)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const float speed = Random::Float(
				burst.minimumSpeed * StationExplosionDebrisMinimumSpeedFraction,
				burst.maximumSpeed * StationExplosionDebrisMaximumSpeedFraction);
			const float angle = std::atan2(direction.y, direction.x);
			const float lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime);

			ParticleSpawn spawn{ MakeSpawn(debrisPreset, position, direction * speed, angle) };
			spawn.lifetime = lifetime;

			impactParticles.Emit(spawn);
		}

		const auto& smokePreset{ Preset("station_explosion_smoke") };
		for (int index = 0; index < smokePreset.count; ++index)
		{
			const sf::Vector2f direction{ VectorMath::RandomDirection() };
			const sf::Vector2f spawnPosition{
				position + direction * Random::Float(smokePreset.spawnOffset.minimum, smokePreset.spawnOffset.maximum) };
			const sf::Vector2f velocity{ direction * Random::Float(smokePreset.speed.minimum, smokePreset.speed.maximum) };
			const float rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);

			smokeParticles.Emit(MakeSpawn(smokePreset, spawnPosition, velocity, rotation, scale, scale));
		}

		shockwaveParticles.Emit(MakeSpawn(Preset("station_explosion_shockwave"), position, {}, 0.f, 1.f, scale));

		StartCameraShake(config.largeExplosionShake, scale * 1.7f);
		StartShockwave(position, 2.05f, 1.55f);
	}

	void GameplayEffects::EmitScorePopup(const sf::Vector2f& position, int points)
	{
		scorePopups.emplace_back(scorePopupFont, points, position);
	}

	void GameplayEffects::UpdateScorePopups(float deltaTime)
	{
		for (ScorePopup& popup : scorePopups)
		{
			popup.elapsedSeconds += deltaTime;

			const float progress = std::clamp(popup.elapsedSeconds / ScorePopupDuration, 0.f, 1.f);
			popup.text.move({ 0.f, -ScorePopupRiseDistance * deltaTime / ScorePopupDuration });
			const float fade = 1.f - progress * progress;
			const std::uint8_t alpha = static_cast<std::uint8_t>(255.f * fade);

			popup.text.setFillColor(sf::Color(225, 252, 255, alpha));
			popup.text.setOutlineColor(sf::Color(20, 175, 255, static_cast<std::uint8_t>(230.f * fade)));
		}

		std::erase_if(scorePopups, [](const ScorePopup& popup)
			{
				return popup.elapsedSeconds >= ScorePopupDuration;
			});
	}

	void GameplayEffects::StartCameraShake(const GameplayData::CameraShakeConfig& shake, float scale)
	{
		if (!isShakeEnabled)
			return;

		shakeDuration = std::max(shakeDuration, shake.duration);
		shakeRemaining = std::max(shakeRemaining, shake.duration);
		shakeAmplitude = std::min(12.f, std::max(shakeAmplitude, shake.amplitude * scale));
	}

	void GameplayEffects::UpdateCameraShake(float deltaTime)
	{
		if (shakeRemaining <= 0.f || shakeDuration <= 0.f)
		{
			cameraOffset = {};
			shakeAmplitude = 0.f;
			return;
		}

		shakeRemaining = std::max(0.f, shakeRemaining - deltaTime);

		const float FallOff = shakeRemaining / shakeDuration;
		const sf::Vector2f target
		{
			Random::Float(-shakeAmplitude, shakeAmplitude) * FallOff,
			Random::Float(-shakeAmplitude, shakeAmplitude) * FallOff
		};

		cameraOffset = cameraOffset * 0.3f + target * 0.7f;
	}

	void GameplayEffects::StartShockwave(const sf::Vector2f& position, float scale, float strength)
	{
		if (shockwaves.size() >= MaximumShockwaves)
			shockwaves.erase(shockwaves.begin());

		shockwaves.push_back({
			position,
			0.f,
			0.55f,
			std::clamp(scale, 0.7f, 2.25f),
			std::clamp(strength, 0.1f, 1.75f) });
	}

	void GameplayEffects::UpdatePostProcess(float deltaTime)
	{
		postProcessState.damageVignette = std::max(0.f, postProcessState.damageVignette - deltaTime * 2.6f);

		for (Shockwave& shockwave : shockwaves)
			shockwave.elapsedSeconds += deltaTime;

		std::erase_if(shockwaves, [](const Shockwave& shockwave)
			{
				return shockwave.elapsedSeconds >= shockwave.duration;
			});

		postProcessState.shockwaveCount = std::min(shockwaves.size(), MaximumShockwaves);

		for (std::size_t index = 0u; index < postProcessState.shockwaveCount; index++)
		{
			const Shockwave& shockwave = shockwaves[index];
			const float progress = std::clamp(shockwave.elapsedSeconds / shockwave.duration, 0.f, 1.f);

			// The explosion particles are rendered with the current camera-shake
			// transform, so the post-process center must use the same offset.
			postProcessState.shockwavePositions[index] = shockwave.position + cameraOffset;
			postProcessState.shockwaveRadii[index] = std::lerp(24.f, 285.f * shockwave.scale, progress);
			postProcessState.shockwaveStrengths[index] = (1.f - progress) * shockwave.strength;
		}
	}
}