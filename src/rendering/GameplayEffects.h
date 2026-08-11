#pragma once

#include <cstddef>
#include <random>

#include "game/GameplayData.h"
#include "ParticleSystem.h"

class World;

namespace sf
{
    class RenderTarget;
    struct RenderStates;
}

class GameplayEffects
{
public:
    explicit GameplayEffects(const GameplayData::EffectsConfig& config);

    void Update(float deltaTime, World& world);
    void DrawBehindEntities(sf::RenderTarget& target, sf::RenderStates states) const;
    void DrawAboveEntities(sf::RenderTarget& target, sf::RenderStates states) const;
    void Clear();

    [[nodiscard]] std::size_t GetParticleCount() const noexcept;
    [[nodiscard]] sf::Vector2f GetCameraOffset() const noexcept;

private:
    float RandomFloat(float minimum, float maximum);
    sf::Vector2f RandomDirection();
    sf::Vector2f RandomDirectionAround(const sf::Vector2f& direction, float spreadRadians);
    void EmitPlayerEngineParticles(const World& world);
    void EmitProjectileGlow(bool playerProjectile, const sf::Vector2f& position,
        const sf::Vector2f& direction);
    void EmitMuzzleFlash(bool playerProjectile, const sf::Vector2f& position,
        const sf::Vector2f& direction);
    void EmitStoneHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
    void EmitMetalHit(const sf::Vector2f& position, const sf::Vector2f& direction, float scale);
    void EmitAsteroidExplosion(const sf::Vector2f& position, float scale);
    void EmitShipExplosion(const sf::Vector2f& position, float scale);
    void StartCameraShake(const GameplayData::CameraShakeConfig& shake, float scale);
    void UpdateCameraShake(float deltaTime);

    const GameplayData::EffectsConfig& config;
    ParticleSystem engineParticles;
    ParticleSystem projectileGlowParticles;
    ParticleSystem weaponParticles;
    ParticleSystem impactParticles;
    ParticleSystem smokeParticles{ ParticleAppearance::Smoke };
    ParticleSystem debrisParticles{ ParticleAppearance::Debris };
    ParticleSystem shockwaveParticles{ ParticleAppearance::Ring };
    std::mt19937 random{ 0x51A7F00Du };
    float engineEmissionAccumulator{ 0.f };
    float shakeRemaining{ 0.f };
    float shakeDuration{ 0.f };
    float shakeAmplitude{ 0.f };
    sf::Vector2f cameraOffset{};
};
