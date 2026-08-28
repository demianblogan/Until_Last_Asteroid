#include "GameplayEffects.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "entities/Player.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"

namespace Rendering
{
	namespace
	{
		constexpr float NormalizeEpsilonSquared = 0.0001f;

		sf::Vector2f Normalize(const sf::Vector2f& vector)
		{
			const float lengthSquared = vector.x * vector.x + vector.y * vector.y;
			if (lengthSquared <= NormalizeEpsilonSquared)
				return { 0.f, -1.f };

			return vector / std::sqrt(lengthSquared);
		}

		int ScaledCount(int count, float scale)
		{
			return std::max(1, static_cast<int>(std::round(static_cast<float>(count) * scale)));
		}

		constexpr float EngineEmissionRateHz = 70.f;
		constexpr float EngineEmissionIntervalSeconds = 1.f / EngineEmissionRateHz;
		constexpr int MaximumEngineEmissionCatchUpSteps = 8;

		constexpr int WeldingSparkCount = 5;
		constexpr int StationChainExplosionDebrisCount = 10;
		constexpr int StationChainExplosionSmokeCount = 2;
		constexpr int PlayerTeleportSparkCount = 28;
		constexpr int MuzzleFlashSparkCount = 8;

		constexpr int StoneHitMinimumSmokeCount = 1;
		constexpr int StoneHitSmokeCountDivisor = 5;

		constexpr int AsteroidExplosionMinimumSmokeCount = 3;
		constexpr int AsteroidExplosionSmokeCountDivisor = 5;

		constexpr int ShipExplosionMinimumSmokeCount = 4;
		constexpr int ShipExplosionSmokeCountDivisor = 6;

		constexpr float StationExplosionDebrisScaleMultiplier = 1.45f;
		constexpr int StationExplosionSmokeCount = 14;

		constexpr float ScorePopupDuration = 0.5f;
		constexpr float ScorePopupRiseDistance = 56.f;
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
				EmitMetalHit(event.position, event.direction, event.scale);
				postProcessState.damageVignette = 1.f;
				StartCameraShake(config.damageShake, event.scale);
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
		projectileGlowParticles.Update(0.f);
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

	sf::Vector2f GameplayEffects::GenerateRandomDirection()
	{
		const float angle = Random::Float(0.f, 2.f * std::numbers::pi_v<float>);
		return { std::cos(angle), std::sin(angle) };
	}

	sf::Vector2f GameplayEffects::GenerateRandomDirectionAround(const sf::Vector2f& direction, float spreadRadians)
	{
		const sf::Vector2f normalized{ Normalize(direction) };
		const float angle = std::atan2(normalized.y, normalized.x) + Random::Float(-spreadRadians, spreadRadians);
		return { std::cos(angle), std::sin(angle) };
	}

	void GameplayEffects::EmitPlayerEngineParticles(const World& world)
	{
		Player* player = world.GetPlayer();
		const auto playerState = player ? player->GetEffectState() : std::nullopt;

		if (!playerState)
			return;

		const sf::Vector2f perpendicular{ -playerState->exhaustDirection.y,	playerState->exhaustDirection.x };

		for (const sf::Vector2f& emitterPosition : playerState->enginePositions)
		{
			const float sidewaysJitter = Random::Float(-24.f, 24.f);
			const float exhaustSpeed = Random::Float(150.f, 235.f);
			const float lifetime = Random::Float(0.2f, 0.32f);

			const ParticleSpawn spawn
			{
				.position = emitterPosition + perpendicular * Random::Float(-1.5f, 1.5f),
				.velocity = playerState->velocity * 0.18f + playerState->exhaustDirection * exhaustSpeed +
					perpendicular * sidewaysJitter,
				.lifetime = lifetime,
				.startSize = Random::Float(14.f, 19.f),
				.endSize = Random::Float(2.f, 4.f),
				.startColor = { 155, 255, 255, 250 },
				.endColor = { 25, 95, 255, 0 },
				.drag = 2.5f
			};

			engineParticles.Emit(spawn);
		}
	}

	void GameplayEffects::EmitProjectileGlow(bool isPlayerProjectile, const sf::Vector2f& position,
		bool isHomingProjectile, bool isTripleShot)
	{
		sf::Color startColor;
		if (isTripleShot)
			startColor = { 65, 255, 115, 240 };
		else if (isHomingProjectile)
			startColor = { 255, 188, 45, 240 };
		else if (isPlayerProjectile)
			startColor = { 85, 235, 255, 235 };
		else
			startColor = { 255, 55, 110, 235 };

		const ParticleSpawn spawn
		{
			.position = position,
			.lifetime = 1.f,
			.startSize = isPlayerProjectile ? 34.f : 31.f,
			.endSize = isPlayerProjectile ? 34.f : 31.f,
			.startColor = startColor,
			.endColor = startColor
		};

		projectileGlowParticles.Emit(spawn);
	}

	void GameplayEffects::EmitMissileSmoke(const sf::Vector2f& position, const sf::Vector2f& direction)
	{
		const sf::Vector2f normalized{ Normalize(direction) };
		const sf::Vector2f perpendicular{ -normalized.y, normalized.x };
		const sf::Vector2f exhaustPosition{ position - normalized * 20.f };

		const ParticleSpawn smokeSpawn
		{
			.position = exhaustPosition + perpendicular * Random::Float(-2.5f, 2.5f),
			.velocity = -normalized * Random::Float(35.f, 65.f) + perpendicular * Random::Float(-22.f, 22.f),
			.lifetime = Random::Float(0.38f, 0.62f),
			.startSize = Random::Float(13.f, 19.f),
			.endSize = Random::Float(25.f, 34.f),
			.startColor = { 125, 130, 145, 155 },
			.endColor = { 35, 38, 48, 0 },
			.angularVelocity = Random::Float(-0.8f, 0.8f),
			.drag = 1.2f
		};

		smokeParticles.Emit(smokeSpawn);

		const ParticleSpawn flareSpawn
		{
			.position = exhaustPosition,
			.velocity = -normalized * Random::Float(30.f, 55.f),
			.lifetime = Random::Float(0.08f, 0.13f),
			.startSize = Random::Float(13.f, 17.f),
			.endSize = Random::Float(4.f, 7.f),
			.startColor = { 255, 245, 135, 255 },
			.endColor = { 255, 90, 15, 0 },
			.drag = 2.f
		};

		weaponParticles.Emit(flareSpawn);
	}

	void GameplayEffects::EmitEnemyEngine(const sf::Vector2f& position, const sf::Vector2f& direction)
	{
		const sf::Vector2f normalized{ Normalize(direction) };
		const sf::Vector2f perpendicular{ -normalized.y, normalized.x };

		const ParticleSpawn spawn
		{
			.position = position + perpendicular * Random::Float(-1.5f, 1.5f),
			.velocity = normalized * Random::Float(125.f, 190.f) + perpendicular * Random::Float(-20.f, 20.f),
			.lifetime = Random::Float(0.18f, 0.28f),
			.startSize = Random::Float(11.f, 16.f),
			.endSize = Random::Float(2.f, 4.f),
			.startColor = { 255, 105, 75, 245 },
			.endColor = { 130, 12, 28, 0 },
			.drag = 2.2f
		};

		engineParticles.Emit(spawn);
	}

	void GameplayEffects::EmitStationWelding(const sf::Vector2f& position, float scale)
	{
		const sf::Vector2f source =
		{
			position.x + Random::Float(-26.f, 26.f) * scale,
			position.y + Random::Float(-26.f, 26.f) * scale
		};

		const ParticleSpawn glowSpawn
		{
			.position = source,
			.lifetime = Random::Float(0.06f, 0.1f),
			.startSize = Random::Float(18.f, 30.f) * scale,
			.endSize = 2.f,
			.startColor = { 225, 250, 255, 255 },
			.endColor = { 45, 145, 255, 0 },
			.drag = 3.f
		};

		impactParticles.Emit(glowSpawn);

		for (int index = 0; index < WeldingSparkCount; index++)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const float angle = std::atan2(direction.y, direction.x);

			const ParticleSpawn sparkSpawn
			{
				.position = source,
				.velocity = direction * Random::Float(95.f, 240.f),
				.lifetime = Random::Float(0.12f, 0.28f),
				.startSize = Random::Float(3.f, 7.f) * scale,
				.endSize = 0.7f,
				.startColor = { 225, 250, 255, 255 },
				.endColor = { 255, 90, 20, 0 },
				.rotation = angle,
				.drag = 0.7f,
				.aspectRatio = Random::Float(2.5f, 5.f)
			};

			impactParticles.Emit(sparkSpawn);
		}
	}

	void GameplayEffects::EmitStationChainExplosion(const sf::Vector2f& position, float scale)
	{
		const ParticleSpawn coreSpawn
		{
			.position = position,
			.lifetime = 0.24f,
			.startSize = 90.f * scale,
			.endSize = 9.f,
			.startColor = { 255, 235, 170, 255 },
			.endColor = { 255, 45, 10, 0 }
		};

		impactParticles.Emit(coreSpawn);

		const ParticleSpawn glowSpawn
		{
			.position = position,
			.lifetime = 0.42f,
			.startSize = 50.f * scale,
			.endSize = 125.f * scale,
			.startColor = { 255, 85, 20, 190 },
			.endColor = { 90, 10, 5, 0 }
		};

		impactParticles.Emit(glowSpawn);

		for (int index = 0; index < StationChainExplosionDebrisCount; index++)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const float angle = std::atan2(direction.y, direction.x);
			const ParticleSpawn debrisSpawn
			{
				.position = position,
				.velocity = direction * Random::Float(90.f, 280.f),
				.lifetime = Random::Float(0.18f, 0.42f),
				.startSize = Random::Float(3.f, 8.f) * scale,
				.endSize = 0.8f,
				.startColor = { 255, 225, 135, 255 },
				.endColor = { 255, 35, 8, 0 },
				.rotation = angle,
				.drag = 0.8f,
				.aspectRatio = Random::Float(2.f, 4.5f)
			};

			impactParticles.Emit(debrisSpawn);
		}

		for (int index = 0; index < StationChainExplosionSmokeCount; index++)
		{
			const ParticleSpawn smokeSpawn
			{
				.position = position + GenerateRandomDirection() * Random::Float(0.f, 10.f),
				.velocity = GenerateRandomDirection() * Random::Float(20.f, 60.f),
				.lifetime = Random::Float(0.5f, 0.9f),
				.startSize = Random::Float(30.f, 46.f) * scale,
				.endSize = Random::Float(68.f, 98.f) * scale,
				.startColor = { 95, 65, 55, 170 },
				.endColor = { 22, 20, 24, 0 }
			};

			smokeParticles.Emit(smokeSpawn);
		}
	}

	void GameplayEffects::EmitPlayerTeleport(const sf::Vector2f& position, float scale)
	{
		const ParticleSpawn shockwaveSpawn
		{
			.position = position,
			.lifetime = 0.5f,
			.startSize = 28.f * scale,
			.endSize = 175.f * scale,
			.startColor = { 165, 255, 255, 245 },
			.endColor = { 20, 105, 255, 0 }
		};

		shockwaveParticles.Emit(shockwaveSpawn);

		const ParticleSpawn flashSpawn
		{
			.position = position,
			.lifetime = 0.3f,
			.startSize = 82.f * scale,
			.endSize = 10.f,
			.startColor = { 225, 255, 255, 255 },
			.endColor = { 25, 145, 255, 0 }
		};

		impactParticles.Emit(flashSpawn);

		for (int index = 0; index < PlayerTeleportSparkCount; ++index)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const float angle = std::atan2(direction.y, direction.x);
			const ParticleSpawn sparkSpawn
			{
				.position = position + direction * Random::Float(12.f, 55.f) * scale,
				.velocity = direction * Random::Float(70.f, 230.f),
				.lifetime = Random::Float(0.25f, 0.55f),
				.startSize = Random::Float(4.f, 9.f) * scale,
				.endSize = 0.8f,
				.startColor = { 205, 255, 255, 255 },
				.endColor = { 25, 90, 255, 0 },
				.rotation = angle,
				.drag = 0.8f,
				.aspectRatio = Random::Float(2.f, 4.f)
			};

			impactParticles.Emit(sparkSpawn);
		}
	}

	void GameplayEffects::EmitMuzzleFlash(bool isPlayerProjectile, const sf::Vector2f& position,
		const sf::Vector2f& direction, float scale)
	{
		const sf::Color coreColor{ isPlayerProjectile ? sf::Color{ 185, 255, 255, 255 } : sf::Color{ 255, 175, 195, 255 } };
		const sf::Color fadeColor{ isPlayerProjectile ? sf::Color{ 30, 135, 255, 0 } : sf::Color{ 255, 25, 75, 0 } };

		const ParticleSpawn coreSpawn
		{
			.position = position + direction * 3.f,
			.velocity = direction * Random::Float(15.f, 35.f),
			.lifetime = Random::Float(0.14f, 0.18f),
			.startSize = (isPlayerProjectile ? 58.f : 50.f) * scale,
			.endSize = 5.f * scale,
			.startColor = coreColor,
			.endColor = fadeColor,
			.drag = 5.f
		};

		weaponParticles.Emit(coreSpawn);

		const sf::Vector2f perpendicular{ -direction.y, direction.x };
		const ParticleSpawn haloSpawn
		{
			.position = position + direction * 7.f,
			.velocity = direction * Random::Float(35.f, 65.f),
			.lifetime = Random::Float(0.1f, 0.14f),
			.startSize = (isPlayerProjectile ? 27.f : 24.f) * scale,
			.endSize = 2.f * scale,
			.startColor = sf::Color::White,
			.endColor = fadeColor
		};

		weaponParticles.Emit(haloSpawn);

		for (int i = 0; i < ScaledCount(MuzzleFlashSparkCount, scale); i++)
		{
			const ParticleSpawn sparkSpawn
			{
				.position = position,
				.velocity = direction * Random::Float(80.f, 175.f) + perpendicular * Random::Float(-95.f, 95.f),
				.lifetime = Random::Float(0.08f, 0.16f),
				.startSize = Random::Float(5.f, 9.f) * scale,
				.endSize = Random::Float(1.f, 2.5f) * scale,
				.startColor = coreColor,
				.endColor = fadeColor,
				.drag = 3.f
			};

			weaponParticles.Emit(sparkSpawn);
		}
	}

	void GameplayEffects::EmitStoneHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale)
	{
		const auto& burst = config.stoneHit;
		const ParticleSpawn flashSpawn
		{
			.position = position,
			.lifetime = 0.16f,
			.startSize = 34.f * scale,
			.endSize = 5.f,
			.startColor = { 255, 210, 125, 220 },
			.endColor = { 150, 80, 25, 0 }
		};

		impactParticles.Emit(flashSpawn);

		for (int i = 0; i < ScaledCount(burst.count, scale); i++)
		{
			const sf::Vector2f particleDirection{ GenerateRandomDirectionAround(direction, 1.35f) };
			const float size = Random::Float(burst.minimumSize, burst.maximumSize) * scale;
			const ParticleSpawn debrisSpawn
			{
				.position = position,
				.velocity = particleDirection * Random::Float(burst.minimumSpeed, burst.maximumSpeed),
				.lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime),
				.startSize = size,
				.endSize = size * 0.45f,
				.startColor = { 185, 150, 105, 245 },
				.endColor = { 75, 55, 40, 0 },
				.rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>),
				.angularVelocity = Random::Float(-8.f, 8.f),
				.drag = 2.2f
			};

			debrisParticles.Emit(debrisSpawn);
		}

		for (int i = 0; i < std::max(StoneHitMinimumSmokeCount, ScaledCount(burst.count, scale) / StoneHitSmokeCountDivisor); i++)
		{
			const ParticleSpawn dustSpawn
			{
				.position = position + GenerateRandomDirection() * Random::Float(0.f, 8.f),
				.velocity = GenerateRandomDirectionAround(direction, 1.5f) * Random::Float(20.f, 65.f),
				.lifetime = Random::Float(0.3f, 0.55f),
				.startSize = Random::Float(18.f, 28.f) * scale,
				.endSize = Random::Float(32.f, 48.f) * scale,
				.startColor = { 135, 115, 90, 150 },
				.endColor = { 45, 40, 38, 0 },
				.drag = 1.4f
			};

			smokeParticles.Emit(dustSpawn);
		}
	}

	void GameplayEffects::EmitMetalHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale)
	{
		const auto& burst = config.metalHit;
		const ParticleSpawn flashSpawn
		{
			.position = position,
			.lifetime = 0.13f,
			.startSize = 42.f * scale,
			.endSize = 3.f,
			.startColor = { 220, 250, 255, 255 },
			.endColor = { 30, 145, 255, 0 }
		};

		impactParticles.Emit(flashSpawn);

		for (int i = 0; i < ScaledCount(burst.count, scale); i++)
		{
			const sf::Vector2f sparkDirection{ GenerateRandomDirectionAround(direction, 1.55f) };
			const float speed = Random::Float(burst.minimumSpeed, burst.maximumSpeed);
			const float angle = std::atan2(sparkDirection.y, sparkDirection.x);
			const ParticleSpawn sparkSpawn
			{
				.position = position,
				.velocity = sparkDirection * speed,
				.lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime),
				.startSize = Random::Float(burst.minimumSize, burst.maximumSize) * scale,
				.endSize = 0.8f,
				.startColor = { 210, 250, 255, 255 },
				.endColor = { 20, 110, 255, 0 },
				.rotation = angle,
				.drag = 0.8f,
				.aspectRatio = Random::Float(2.5f, 4.5f)
			};

			impactParticles.Emit(sparkSpawn);
		}
	}

	void GameplayEffects::EmitAsteroidExplosion(const sf::Vector2f& position, float scale)
	{
		const bool isLarge = scale >= 0.8f;
		const auto& burst = isLarge ? config.largeAsteroidExplosion : config.smallAsteroidExplosion;
		const ParticleSpawn flashSpawn
		{
			.position = position,
			.lifetime = 0.34f,
			.startSize = 145.f * scale,
			.endSize = 8.f,
			.startColor = { 255, 235, 175, 255 },
			.endColor = { 255, 80, 15, 0 }
		};

		impactParticles.Emit(flashSpawn);

		const ParticleSpawn glowSpawn
		{
			.position = position,
			.lifetime = 0.5f,
			.startSize = 85.f * scale,
			.endSize = 185.f * scale,
			.startColor = { 255, 120, 30, 190 },
			.endColor = { 120, 25, 5, 0 }
		};

		impactParticles.Emit(glowSpawn);

		for (int i = 0; i < ScaledCount(burst.count, scale); i++)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const float size = Random::Float(burst.minimumSize, burst.maximumSize);
			const ParticleSpawn debrisSpawn
			{
				.position = position + direction * Random::Float(0.f, 10.f * scale),
				.velocity = direction * Random::Float(burst.minimumSpeed, burst.maximumSpeed),
				.lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime),
				.startSize = size,
				.endSize = size * 0.55f,
				.startColor = { 195, 155, 105, 255 },
				.endColor = { 55, 42, 35, 0 },
				.rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>),
				.angularVelocity = Random::Float(-7.f, 7.f),
				.drag = 1.6f
			};

			debrisParticles.Emit(debrisSpawn);
		}

		const int smokeCount = std::max(AsteroidExplosionMinimumSmokeCount, ScaledCount(burst.count, scale) / AsteroidExplosionSmokeCountDivisor);
		for (int i = 0; i < smokeCount; i++)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const ParticleSpawn smokeSpawn
			{
				.position = position + direction * Random::Float(0.f, 22.f * scale),
				.velocity = direction * Random::Float(25.f, 95.f),
				.lifetime = Random::Float(0.65f, 1.25f),
				.startSize = Random::Float(28.f, 50.f) * scale,
				.endSize = Random::Float(65.f, 105.f) * scale,
				.startColor = { 100, 82, 70, 185 },
				.endColor = { 28, 27, 30, 0 },
				.rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>),
				.angularVelocity = Random::Float(-1.3f, 1.3f),
				.drag = 0.9f
			};

			smokeParticles.Emit(smokeSpawn);
		}

		if (isLarge)
		{
			const ParticleSpawn shockwaveSpawn
			{
				.position = position,
				.lifetime = 0.42f,
				.startSize = 42.f,
				.endSize = 245.f * scale,
				.startColor = { 255, 190, 95, 205 },
				.endColor = { 255, 80, 20, 0 }
			};

			shockwaveParticles.Emit(shockwaveSpawn);

			StartCameraShake(config.largeExplosionShake, 0.8f * scale);
			StartShockwave(position, scale);
		}
	}

	void GameplayEffects::EmitShipExplosion(const sf::Vector2f& position, float scale)
	{
		const auto& burst = config.shipExplosion;
		const ParticleSpawn flashSpawn
		{
			.position = position,
			.lifetime = 0.28f,
			.startSize = 165.f * scale,
			.endSize = 9.f,
			.startColor = { 235, 255, 255, 255 },
			.endColor = { 25, 130, 255, 0 }
		};

		impactParticles.Emit(flashSpawn);

		const ParticleSpawn glowSpawn
		{
			.position = position,
			.lifetime = 0.52f,
			.startSize = 75.f * scale,
			.endSize = 205.f * scale,
			.startColor = { 80, 210, 255, 210 },
			.endColor = { 255, 40, 20, 0 }
		};

		impactParticles.Emit(glowSpawn);

		for (int i = 0; i < ScaledCount(burst.count, scale); ++i)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const float speed = Random::Float(burst.minimumSpeed, burst.maximumSpeed);
			if (i % 2 == 0)
			{
				const float angle = std::atan2(direction.y, direction.x);
				const ParticleSpawn sparkSpawn
				{
					.position = position,
					.velocity = direction * speed,
					.lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime) * 0.65f,
					.startSize = Random::Float(2.f, 5.f),
					.endSize = 0.7f,
					.startColor = { 220, 250, 255, 255 },
					.endColor = { 255, 65, 20, 0 },
					.rotation = angle,
					.drag = 0.8f,
					.aspectRatio = Random::Float(2.5f, 5.f)
				};

				impactParticles.Emit(sparkSpawn);
			}
			else
			{
				const float size = Random::Float(burst.minimumSize, burst.maximumSize);
				const ParticleSpawn debrisSpawn
				{
					.position = position,
					.velocity = direction * speed * 0.72f,
					.lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime),
					.startSize = size,
					.endSize = size * 0.5f,
					.startColor = { 135, 175, 190, 255 },
					.endColor = { 45, 52, 60, 0 },
					.rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>),
					.angularVelocity = Random::Float(-11.f, 11.f),
					.drag = 1.2f
				};

				debrisParticles.Emit(debrisSpawn);
			}
		}

		const int smokeCount = std::max(ShipExplosionMinimumSmokeCount, ScaledCount(burst.count, scale) / ShipExplosionSmokeCountDivisor);
		for (int i = 0; i < smokeCount; i++)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const ParticleSpawn smokeSpawn
			{
				.position = position + direction * Random::Float(0.f, 20.f * scale),
				.velocity = direction * Random::Float(25.f, 105.f),
				.lifetime = Random::Float(0.7f, 1.35f),
				.startSize = Random::Float(26.f, 48.f) * scale,
				.endSize = Random::Float(70.f, 115.f) * scale,
				.startColor = { 80, 88, 98, 190 },
				.endColor = { 20, 23, 30, 0 },
				.rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>),
				.angularVelocity = Random::Float(-1.5f, 1.5f),
				.drag = 0.8f
			};

			smokeParticles.Emit(smokeSpawn);
		}

		const ParticleSpawn shockwaveSpawn
		{
			.position = position,
			.lifetime = 0.46f,
			.startSize = 48.f,
			.endSize = 265.f * scale,
			.startColor = { 120, 225, 255, 220 },
			.endColor = { 255, 65, 30, 0 }
		};

		shockwaveParticles.Emit(shockwaveSpawn);

		StartCameraShake(config.largeExplosionShake, scale);
		StartShockwave(position, scale);
	}

	void GameplayEffects::EmitStationExplosion(const sf::Vector2f& position, float scale)
	{
		const auto& burst = config.shipExplosion;
		const ParticleSpawn flashSpawn
		{
			.position = position,
			.lifetime = 0.42f,
			.startSize = 230.f * scale,
			.endSize = 14.f,
			.startColor = { 255, 245, 215, 255 },
			.endColor = { 255, 45, 15, 0 }
		};

		impactParticles.Emit(flashSpawn);

		const ParticleSpawn glowSpawn
		{
			.position = position,
			.lifetime = 0.72f,
			.startSize = 120.f * scale,
			.endSize = 320.f * scale,
			.startColor = { 255, 85, 25, 225 },
			.endColor = { 75, 5, 12, 0 }
		};

		impactParticles.Emit(glowSpawn);

		for (int index = 0; index < ScaledCount(burst.count, scale * StationExplosionDebrisScaleMultiplier); index++)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const float speed = Random::Float(burst.minimumSpeed * 0.9f, burst.maximumSpeed * 1.3f);
			const float angle = std::atan2(direction.y, direction.x);
			const ParticleSpawn debrisSpawn
			{
				.position = position,
				.velocity = direction * speed,
				.lifetime = Random::Float(burst.minimumLifetime, burst.maximumLifetime),
				.startSize = Random::Float(4.f, 14.f),
				.endSize = 0.9f,
				.startColor = { 255, 225, 170, 255 },
				.endColor = { 255, 30, 12, 0 },
				.rotation = angle,
				.angularVelocity = Random::Float(-6.f, 6.f),
				.drag = 0.8f,
				.aspectRatio = Random::Float(2.f, 5.f)
			};

			impactParticles.Emit(debrisSpawn);
		}

		for (int index = 0; index < StationExplosionSmokeCount; ++index)
		{
			const sf::Vector2f direction{ GenerateRandomDirection() };
			const ParticleSpawn smokeSpawn
			{
				.position = position + direction * Random::Float(0.f, 55.f),
				.velocity = direction * Random::Float(35.f, 145.f),
				.lifetime = Random::Float(0.9f, 1.7f),
				.startSize = Random::Float(38.f, 68.f) * scale,
				.endSize = Random::Float(95.f, 155.f) * scale,
				.startColor = { 105, 72, 62, 205 },
				.endColor = { 18, 18, 24, 0 },
				.rotation = Random::Float(0.f, 2.f * std::numbers::pi_v<float>),
				.angularVelocity = Random::Float(-1.5f, 1.5f),
				.drag = 0.8f
			};

			smokeParticles.Emit(smokeSpawn);
		}

		const ParticleSpawn shockwaveSpawn
		{
			.position = position,
			.lifetime = 0.7f,
			.startSize = 75.f,
			.endSize = 520.f * scale,
			.startColor = { 255, 155, 80, 240 },
			.endColor = { 255, 25, 15, 0 }
		};

		shockwaveParticles.Emit(shockwaveSpawn);

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