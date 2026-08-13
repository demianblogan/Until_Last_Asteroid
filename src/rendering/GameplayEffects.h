#pragma once

#include <cstddef>
#include <array>
#include <random>
#include <vector>

#include <SFML/Graphics/Text.hpp>

#include "game/GameplayData.h"
#include "ParticleSystem.h"

class World;
class AssetStore;

namespace sf
{
    class RenderTarget;
    struct RenderStates;
}

class GameplayEffects
{
public:
	static constexpr std::size_t MaximumShockwaves{ 8u };

    struct PostProcessState
    {
		std::array<sf::Vector2f, MaximumShockwaves> shockwavePositions{};
		std::array<float, MaximumShockwaves> shockwaveRadii{};
		std::array<float, MaximumShockwaves> shockwaveStrengths{};
		std::size_t shockwaveCount{ 0u };
        float damageVignette{ 0.f };
    };

    GameplayEffects(const GameplayData::EffectsConfig& config, AssetStore& assets);

    void Update(float deltaTime, World& world, bool screenShakeEnabled, bool showScorePopups);
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
        float elapsed{ 0.f };
    };

	struct Shockwave
	{
		sf::Vector2f position;
		float elapsed{ 0.f };
		float duration{ 0.55f };
		float scale{ 1.f };
	};

    float RandomFloat(float minimum, float maximum);
    sf::Vector2f RandomDirection();
    sf::Vector2f RandomDirectionAround(const sf::Vector2f& direction, float spreadRadians);
    void EmitPlayerEngineParticles(const World& world);
    void EmitProjectileGlow(bool playerProjectile, const sf::Vector2f& position,
        const sf::Vector2f& direction, bool homing = false);
	void EmitMissileSmoke(const sf::Vector2f& position, const sf::Vector2f& direction);
	void EmitEnemyEngine(const sf::Vector2f& position, const sf::Vector2f& direction);
    void EmitMuzzleFlash(bool playerProjectile, const sf::Vector2f& position,
        const sf::Vector2f& direction);
    void EmitStoneHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
    void EmitMetalHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
    void EmitAsteroidExplosion(const sf::Vector2f& position, float scale);
    void EmitShipExplosion(const sf::Vector2f& position, float scale);
    void EmitScorePopup(const sf::Vector2f& position, int points);
    void UpdateScorePopups(float deltaTime);
    void StartCameraShake(const GameplayData::CameraShakeConfig& shake, float scale);
    void UpdateCameraShake(float deltaTime);
    void StartShockwave(const sf::Vector2f& position, float scale);
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
    std::mt19937 random{ 0x51A7F00Du };
    float engineEmissionAccumulator{ 0.f };
    float shakeRemaining{ 0.f };
    float shakeDuration{ 0.f };
    float shakeAmplitude{ 0.f };
    sf::Vector2f cameraOffset{};
    PostProcessState postProcessState;
	std::vector<Shockwave> shockwaves;
    bool shakeEnabled{ true };
};
