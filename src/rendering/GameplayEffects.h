#pragma once

#include <cstddef>
#include <array>
#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "gameplay/GameplayData.h"
#include "ParticleSystem.h"

class World;
class Assets;

namespace sf
{
	class RenderTarget;
	struct RenderStates;
}

namespace Rendering
{
	class GameplayEffects
	{
	public:
		static constexpr std::size_t MaximumShockwaves = 8u;

		struct PostProcessState
		{
			std::array<sf::Vector2f, MaximumShockwaves> shockwavePositions{};
			std::array<float, MaximumShockwaves> shockwaveRadii{};
			std::array<float, MaximumShockwaves> shockwaveStrengths{};
			std::size_t shockwaveCount = 0u;
			float damageVignette = 0.f;
		};

		GameplayEffects(const GameplayData::EffectsConfig& config, Assets& assets);

		void Update(float deltaTime, World& world, bool isScreenShakeEnabled, bool needToShowScorePopups);
		void DrawBehindEntities(sf::RenderTarget& target, sf::RenderStates states) const;
		void DrawAboveEntities(sf::RenderTarget& target, sf::RenderStates states) const;
		void Clear();

		[[nodiscard]] std::size_t GetParticleCount() const noexcept;
		[[nodiscard]] sf::Vector2f GetCameraOffset() const noexcept;
		[[nodiscard]] const PostProcessState& GetPostProcessState() const noexcept;

	private:
		struct ScorePopup
		{
			ScorePopup(const sf::Font& font, int points, sf::Vector2f position);

			sf::Text text;
			float elapsedSeconds = 0.f;
		};

		struct Shockwave
		{
			sf::Vector2f position;
			float elapsedSeconds = 0.f;
			float duration = 0.55f;
			float scale = 1.f;
			float strength = 0.9f;
		};

		// Looks up a named preset from config.particlePresets (see
		// GameplayData::ParticlePresetConfig) -- throws if `name` isn't
		// defined in effects.json, the same fail-fast behavior as every
		// other required gameplay value.
		[[nodiscard]] const GameplayData::ParticlePresetConfig& Preset(const char* name) const;

		// Fills every field of a ParticleSpawn that a preset can supply
		// (lifetime, size, color, drag, spin, aspect ratio); position and
		// velocity are always passed in explicitly since how they're built
		// -- which direction, which jitter -- is each effect's own "shape"
		// and stays in the calling Emit* method rather than becoming data.
		// startSizeScale/endSizeScale default to the same `scale` most
		// callers pass for both; a handful of effects deliberately shrink to
		// a fixed unscaled end size regardless of explosion scale, and pass
		// endSizeScale explicitly for that one field instead.
		[[nodiscard]] ParticleSpawn MakeSpawn(
			const GameplayData::ParticlePresetConfig& preset,
			const sf::Vector2f& position,
			const sf::Vector2f& velocity = {},
			float rotation = 0.f,
			float startSizeScale = 1.f,
			float endSizeScale = 1.f) const;

		void EmitPlayerEngineParticles(const World& world);
		void EmitProjectileGlow(bool isPlayerProjectile, const sf::Vector2f& position,
			bool isHomingProjectile = false, bool isTripleShot = false);
		void EmitMissileSmoke(const sf::Vector2f& position, const sf::Vector2f& direction);
		void EmitEnemyEngine(const sf::Vector2f& position, const sf::Vector2f& direction);
		void EmitStationWelding(const sf::Vector2f& position, float scale);
		void EmitStationChainExplosion(const sf::Vector2f& position, float scale);
		void EmitPlayerTeleport(const sf::Vector2f& position, float scale);
		void EmitMuzzleFlash(bool isPlayerProjectile, const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
		void EmitStoneHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
		void EmitMetalHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
		// EmitMetalHit's particles, plus the extra reaction that's specific to
		// the player taking the hit rather than any other ship: the damage
		// vignette flash and a camera shake.
		void HandlePlayerHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
		void EmitAsteroidExplosion(const sf::Vector2f& position, float scale);
		void EmitShipExplosion(const sf::Vector2f& position, float scale);
		void EmitStationExplosion(const sf::Vector2f& position, float scale);
		void EmitScorePopup(const sf::Vector2f& position, int points);

		void UpdateScorePopups(float deltaTime);

		void StartCameraShake(const GameplayData::CameraShakeConfig& shake, float scale);
		void UpdateCameraShake(float deltaTime);
		void StartShockwave(const sf::Vector2f& position, float scale, float strength = 0.9f);
		void UpdatePostProcess(float deltaTime);

		const GameplayData::EffectsConfig& config;
		const sf::Font& scorePopupFont;

		ParticleSystem engineParticles;
		ParticleSystem projectileGlowParticles;
		ParticleSystem weaponParticles;
		ParticleSystem impactParticles;
		ParticleSystem smokeParticles{ ParticleAppearance::Smoke };
		ParticleSystem debrisParticles{ ParticleAppearance::Debris };
		ParticleSystem shockwaveParticles{ ParticleAppearance::Ring };

		std::vector<ScorePopup> scorePopups;
		float engineEmissionAccumulator = 0.f;
		float shakeRemaining = 0.f;
		float shakeDuration = 0.f;
		float shakeAmplitude = 0.f;

		sf::Vector2f cameraOffset{};
		PostProcessState postProcessState;

		std::vector<Shockwave> shockwaves;
		bool isShakeEnabled = true;
	};
}