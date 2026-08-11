#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "systems/Collision.h"

class GameplayData
{
public:
	struct NormalizedPoint
	{
		float x{ 0.f };
		float y{ 0.f };
	};

    enum class EnemyKind
    {
        BigMeteor,
        SmallMeteor,
        Kamikaze,
        Shooter,
        Count
    };

    enum class ProjectileKind
    {
        Player,
        Enemy,
        Count
    };

    struct PlayerConfig
    {
        int maximumHealth{ 100 };
        int collisionDamage{ 20 };
        float damageInvulnerability{ 1.f };
        float collisionImpulse{ 520.f };
        float acceleration{ 1200.f };
        float damping{ 0.98f };
        float maximumSpeed{ 600.f };
        float shootCooldown{ 0.2f };
        float visualScale{ 1.f };
		float collisionRadius{ 37.5f };
		std::vector<Collision::LocalCircle> collisionCircles;
		std::array<NormalizedPoint, 2> engineEmitters{};
		NormalizedPoint muzzleEmitter{};
    };

    struct EnemyConfig
    {
        int maximumHealth{ 1 };
        int contactDamage{ 0 };
        int score{ 0 };
        float speed{ 0.f };
        float collisionImpulse{ 0.f };
        float actionInterval{ 0.f };
        float fragmentSpeed{ 0.f };
        float soundPitch{ 1.f };
        float visualScale{ 1.f };
        float collisionRadius{ 1.f };
		std::vector<Collision::LocalCircle> collisionCircles;
		float rotationSpeed{ 0.f };
		std::array<NormalizedPoint, 2> weaponEmitters{};
    };

    struct ProjectileConfig
    {
        int damage{ 1 };
        float speed{ 0.f };
        float knockback{ 0.f };
        float visualScale{ 1.f };
        float collisionRadius{ 1.f };
    };

    struct PickupConfig
    {
        float healthRestorePercentage{ 0.25f };
        float shieldCapacity{ 100.f };
        float shieldDuration{ 10.f };
        float visualScale{ 0.075f };
        float collisionRadius{ 42.f };
    };

    struct SpawnGroup
    {
        EnemyKind kind{ EnemyKind::BigMeteor };
        int count{ 0 };
    };

    struct WaveConfig
    {
        struct ScheduledSpawn : SpawnGroup
        {
            float delay{ 0.f };
        };

        std::vector<SpawnGroup> initialSpawns;
        std::vector<ScheduledSpawn> scheduledSpawns;
    };

    struct LevelConfig
    {
        struct PostProcessConfig
        {
            std::array<float, 3> tint{ 1.f, 1.f, 1.f };
            float saturation{ 1.f };
            float contrast{ 1.f };
            float bloomIntensity{ 1.f };
            float vignetteStrength{ 0.22f };
        };

        int number{ 1 };
        std::string background;
        float backgroundBrightness{ 1.f };
        PostProcessConfig postProcess;
        std::vector<WaveConfig> waves;
    };

    struct BurstConfig
    {
        int count{ 1 };
        float minimumSpeed{ 0.f };
        float maximumSpeed{ 1.f };
        float minimumLifetime{ 0.1f };
        float maximumLifetime{ 0.2f };
        float minimumSize{ 1.f };
        float maximumSize{ 2.f };
    };

    struct CameraShakeConfig
    {
        float duration{ 0.1f };
        float amplitude{ 1.f };
    };

    struct EffectsConfig
    {
        BurstConfig stoneHit;
        BurstConfig metalHit;
        BurstConfig smallAsteroidExplosion;
        BurstConfig largeAsteroidExplosion;
        BurstConfig shipExplosion;
        CameraShakeConfig damageShake;
        CameraShakeConfig largeExplosionShake;
    };

    explicit GameplayData(const std::filesystem::path& directory);

    [[nodiscard]] const PlayerConfig& GetPlayer() const noexcept;
    [[nodiscard]] const EnemyConfig& GetEnemy(EnemyKind kind) const noexcept;
    [[nodiscard]] const ProjectileConfig& GetProjectile(ProjectileKind kind) const noexcept;
    [[nodiscard]] const PickupConfig& GetPickups() const noexcept;
    [[nodiscard]] const LevelConfig& GetLevel(int number) const;
    [[nodiscard]] int GetLevelCount() const noexcept;
    [[nodiscard]] float GetHitFlashDuration() const noexcept;
    [[nodiscard]] const EffectsConfig& GetEffects() const noexcept;

private:
    PlayerConfig player;
    std::array<EnemyConfig, static_cast<std::size_t>(EnemyKind::Count)> enemies;
    std::array<ProjectileConfig, static_cast<std::size_t>(ProjectileKind::Count)> projectiles;
    PickupConfig pickups;
    std::vector<LevelConfig> levels;
    float hitFlashDuration{ 0.1f };
    EffectsConfig effects;
};
