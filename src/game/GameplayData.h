#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <vector>

class GameplayData
{
public:
    enum class EnemyKind
    {
        BigMeteor,
        MediumMeteor,
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
    };

    struct ProjectileConfig
    {
        int damage{ 1 };
        float speed{ 0.f };
        float knockback{ 0.f };
    };

    struct SpawnGroup
    {
        EnemyKind kind{ EnemyKind::BigMeteor };
        int count{ 0 };
    };

    struct WaveConfig
    {
        float interval{ 0.f };
        int repetitions{ 0 };
        std::vector<EnemyKind> spawns;
    };

    struct LevelConfig
    {
        int number{ 1 };
        std::vector<SpawnGroup> initialSpawns;
        std::vector<WaveConfig> waves;
    };

    explicit GameplayData(const std::filesystem::path& directory);

    [[nodiscard]] const PlayerConfig& GetPlayer() const noexcept;
    [[nodiscard]] const EnemyConfig& GetEnemy(EnemyKind kind) const noexcept;
    [[nodiscard]] const ProjectileConfig& GetProjectile(ProjectileKind kind) const noexcept;
    [[nodiscard]] const LevelConfig& GetLevel(int number) const;
    [[nodiscard]] int GetLevelCount() const noexcept;
    [[nodiscard]] float GetHitFlashDuration() const noexcept;

private:
    PlayerConfig player;
    std::array<EnemyConfig, static_cast<std::size_t>(EnemyKind::Count)> enemies;
    std::array<ProjectileConfig, static_cast<std::size_t>(ProjectileKind::Count)> projectiles;
    std::vector<LevelConfig> levels;
    float hitFlashDuration{ 0.1f };
};
