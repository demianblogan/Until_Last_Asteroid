#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/Collision.h"

class GameplayData
{
public:
	enum class EncounterKind
	{
		Waves,
		Boss
	};

	struct NormalizedPoint
	{
		float x = 0.f;
		float y = 0.f;
	};

    enum class EnemyKind
    {
        BigMeteor,
        SmallMeteor,
        Kamikaze,
		Shooter,
		Spinner,
		MissileCarrier,
		LaserTurret,
		ShooterStation,
		ReflectorGunship,
        Count
    };

    enum class ProjectileKind
    {
        Player,
		Helper,
        Enemy,
		Spinner,
        Count
    };

	enum class PickupKind
	{
		Health,
		Shield,
		HomingBullets,
		TimeSlowdown,
		Laser,
		TripleShot,
		HelperBot
	};

    struct PlayerConfig
    {
        int maximumHealth = 100;
        int collisionDamage = 20;
        float damageInvulnerability = 1.f;
        float collisionImpulse = 520.f;
        float acceleration = 1200.f;
        float damping = 0.98f;
        float maximumSpeed = 600.f;
        float shootCooldown = 0.2f;
        float visualScale = 1.f;
		float collisionRadius = 37.5f;
		std::vector<Collision::LocalCircle> collisionCircles;
		std::array<NormalizedPoint, 2> engineEmitters{};
		NormalizedPoint muzzleEmitter{};
    };

    struct EnemyConfig
    {
        int maximumHealth = 1;
        int contactDamage = 0;
        int score = 0;
        float speed = 0.f;
        float collisionImpulse = 0.f;
        float actionInterval = 0.f;
        float fragmentSpeed = 0.f;
        float soundPitch = 1.f;
        float visualScale = 1.f;
        float collisionRadius = 1.f;
		std::vector<Collision::LocalCircle> collisionCircles;
		float rotationSpeed = 0.f;
		float sineAmplitude = 0.f;
		float sineFrequency = 0.f;
		int beamDamage = 0;
		float beamWidth = 0.f;
		float shieldDuration = 0.f;
		float spawnAnimationDuration = 0.f;
		std::vector<NormalizedPoint> weaponEmitters;
		std::vector<NormalizedPoint> engineEmitters;
    };

    struct ProjectileConfig
    {
        int damage = 1;
        float speed = 0.f;
        float knockback = 0.f;
        float visualScale = 1.f;
        float collisionRadius = 1.f;
    };

    struct PickupConfig
    {
        float healthRestorePercentage = 0.25f;
        float shieldCapacity = 100.f;
        float shieldDuration = 10.f;
		float homingBulletsDuration = 10.f;
		float homingConeDegrees = 90.f;
		float laserDuration = 10.f;
		float laserDamageInterval = 0.5f;
		float laserWidth = 18.f;
		float tripleShotDuration = 10.f;
		float tripleShotAngleDegrees = 5.f;
		float homingTurnSpeedDegrees = 480.f;
		float timeSlowdownDuration = 5.f;
		float timeSlowdownWorldScale = 0.35f;
		float timeSlowdownAudioPitch = 0.72f;
		float helperBotShotInterval = 0.5f;
		float helperBotTurnSpeedDegrees = 560.f;
		float helperBotOrbitRadius = 90.f;
		float helperBotOrbitSpeedDegrees = 42.f;
		float helperBotVisualScale = 0.045f;
        float visualScale = 0.075f;
        float collisionRadius = 42.f;
    };

	struct PartConfig
	{
		float lifetime = 3.f;
		float blinkDuration = 0.75f;
		float visualScale = 0.065f;
		float collisionRadius = 34.f;
		float rotationSpeedDegrees = 70.f;
	};

	struct MissileConfig
	{
		int maximumHealth = 30;
		int explosionDamage = 40;
		float speed = 280.f;
		float turnSpeedDegrees = 110.f;
		float explosionRadius = 115.f;
		float explosionImpulse = 500.f;
		float lifetime = 12.f;
		float visualScale = 0.07f;
		float collisionRadius = 14.f;
	};

    struct SpawnGroup
    {
		struct PickupDropConfig
		{
			struct WeightedPickup
			{
				PickupKind kind = PickupKind::Health;
				float weight = 1.f;
			};

			float chance = 1.f;
			std::vector<WeightedPickup> pool;
		};

        EnemyKind kind = EnemyKind::BigMeteor;
        int count = 0;
		std::optional<PickupDropConfig> drop;
		std::vector<std::string> partIds;
    };

	struct BossConfig
	{
		int maximumHealth = 3000;
		int contactDamage = 35;
		float outerRingEndHealthRatio = 0.7f;
		float firstShieldHealthRatio = 0.9f;
		float secondShieldHealthRatio = 0.8f;
		float outerRingRotationSpeedDegrees = 8.f;
		float outerRingInnerRadius = 232.f;
		float outerRingOuterRadius = 355.f;
		// Radius of the energy-shield bubble drawn/collided against while the
		// outer ring's shield is up -- a separate knob from outerRingOuterRadius
		// so the shield can be sized independently of the ring sprite itself.
		float outerShieldRadius = 355.f;
		int cannonCount = 6;
		float cannonOrbitRadius = 310.f;
		float cannonFireInterval = 0.6f;
		float cannonStaggerInterval = 0.1f;
		float shieldCycleDuration = 10.f;
		float shieldWarningDuration = 1.5f;
		float shieldWarningBlinkInterval = 0.15f;
		float shieldEnemyClearance = 20.f;
		float kamikazeSpawnInterval = 2.f;
		float shooterSpawnInterval = 3.f;
		float outerRingDestructionDuration = 2.f;
		float outerRingExplosionInterval = 0.14f;
		float diamondRotationSpeedDegrees = 12.f;
		float portalOrbitRadius = 155.f;
		float portalCollisionRadius = 32.f;
		int portalHealth = 300;
		float portalSpawnIntervalPerAlive = 1.f;
		float edgeShooterSpawnInterval = 3.f;
		float innerShieldDuration = 5.f;
		float diamondFrameVertexRadius = 155.f;
		float diamondFrameHalfThickness = 20.f;
		// Radius of the energy-shield bubble for the inner (diamond) phase --
		// same idea as outerShieldRadius, independent of the diamond frame's
		// own geometry above.
		float innerShieldRadius = 225.f;
		float diamondDestructionDuration = 2.f;
		float diamondExplosionInterval = 0.14f;
		float coreShieldRadius = 140.f;
		float coreCollisionRadius = 105.f;
		float coreBeamWidth = 30.f;
		int coreBeamDamage = 10;
		std::array<float, 3> coreBeamSpeeds = { 14.f, 22.f, 32.f };
		float coreAsteroidSpawnInterval = 1.f;
		float coreTurretInset = 105.f;
		float coreStationInset = 145.f;
		std::vector<PickupKind> phaseOneBonusDrops;
		std::vector<PickupKind> phaseTwoBonusDrops;
		std::vector<PickupKind> phaseThreeBonusDrops;
	};

    struct WaveConfig
    {
        struct ScheduledSpawn : SpawnGroup
        {
            float delay = 0.f;
        };

        std::vector<SpawnGroup> initialSpawns;
        std::vector<ScheduledSpawn> scheduledSpawns;
    };

    struct LevelConfig
    {
        struct PostProcessConfig
        {
            std::array<float, 3> tint = { 1.f, 1.f, 1.f };
            float saturation = 1.f;
            float contrast = 1.f;
            float bloomIntensity = 1.f;
            float vignetteStrength = 0.22f;
        };

        int number = 1;
		std::string title;
		EncounterKind encounter = EncounterKind::Waves;
        std::string background;
        float backgroundBrightness = 1.f;
		float targetAccuracyPercent = 75.f;
        PostProcessConfig postProcess;
        std::vector<WaveConfig> waves;
		std::vector<std::string> partIds;
    };

    struct BurstConfig
    {
        int count = 1;
        float minimumSpeed = 0.f;
        float maximumSpeed = 1.f;
        float minimumLifetime = 0.1f;
        float maximumLifetime = 0.2f;
        float minimumSize = 1.f;
        float maximumSize = 2.f;
    };

    struct CameraShakeConfig
    {
        float duration = 0.1f;
        float amplitude = 1.f;
    };

    // A tunable [minimum, maximum] pair -- Random::Float(minimum, maximum) is
    // drawn from it wherever it's used. minimum == maximum for a fixed value.
    struct ParticleRangeConfig
    {
        float minimum = 0.f;
        float maximum = 0.f;
    };

    // Every numeric "look and feel" knob a single particle sub-spawn (one
    // ParticleSpawn's worth of values inside a GameplayEffects::Emit*
    // method) can have, loaded by name from effects.json's "particles"
    // object. What still lives in code, deliberately: which of these fields
    // a given Emit* method actually uses, and the position/velocity formula
    // (e.g. "along the impact direction" vs "a random direction" vs
    // "perpendicular to the exhaust") that combines them -- that's the
    // particle's *shape*, analogous to picking a module in Unity's particle
    // system, and isn't meaningfully expressible as flat data. Everything
    // that IS just a number -- colors, sizes, speeds, drag, spin, how many
    // to spawn -- lives here instead, so retuning an effect's look doesn't
    // need a recompile.
    struct ParticlePresetConfig
    {
        int count = 1;
        ParticleRangeConfig lifetime = { 1.f, 1.f };
        ParticleRangeConfig startSize = { 1.f, 1.f };
        ParticleRangeConfig endSize = { 1.f, 1.f };
        std::array<int, 4> startColor = { 255, 255, 255, 255 };
        std::array<int, 4> endColor = { 255, 255, 255, 255 };
        // Speed along the emitter's main direction (e.g. the impact/exhaust
        // direction, or a random direction -- decided in code per effect).
        ParticleRangeConfig speed = { 0.f, 0.f };
        // Sideways velocity component, along the direction perpendicular to
        // the main one.
        ParticleRangeConfig lateralJitter = { 0.f, 0.f };
        // Position offset applied at spawn -- along whichever direction the
        // calling Emit* method uses for it (perpendicular, main, or random;
        // that choice is the "shape", and stays in code).
        ParticleRangeConfig spawnOffset = { 0.f, 0.f };
        float drag = 0.f;
        ParticleRangeConfig angularVelocity = { 0.f, 0.f };
        ParticleRangeConfig aspectRatio = { 1.f, 1.f };
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
        std::unordered_map<std::string, ParticlePresetConfig> particlePresets;
    };

    explicit GameplayData(const std::filesystem::path& directory);

    [[nodiscard]] const PlayerConfig& GetPlayer() const noexcept;
    [[nodiscard]] const EnemyConfig& GetEnemy(EnemyKind kind) const noexcept;
    [[nodiscard]] const ProjectileConfig& GetProjectile(ProjectileKind kind) const noexcept;
    [[nodiscard]] const PickupConfig& GetPickups() const noexcept;
	[[nodiscard]] const MissileConfig& GetMissile() const noexcept;
	[[nodiscard]] const BossConfig& GetBoss() const noexcept;
	[[nodiscard]] const PartConfig& GetParts() const noexcept;
    [[nodiscard]] const LevelConfig& GetLevel(int number) const;
    [[nodiscard]] int GetLevelCount() const noexcept;
    [[nodiscard]] float GetHitFlashDuration() const noexcept;
    [[nodiscard]] const EffectsConfig& GetEffects() const noexcept;

private:
    // Each loads one gameplay/*.json file into its own section of this
    // object's state. Split out of the constructor (which just calls all
    // seven in order) purely so each file's worth of parsing has its own
    // named, independently-navigable chunk instead of one ~600-line function.
    void LoadBoss(const std::filesystem::path& directory);
    void LoadPlayer(const std::filesystem::path& directory);
    void LoadEnemies(const std::filesystem::path& directory);
    void LoadWeapons(const std::filesystem::path& directory);
    void LoadEffects(const std::filesystem::path& directory);
    void LoadPickups(const std::filesystem::path& directory);
    void LoadLevels(const std::filesystem::path& directory);

    PlayerConfig player;
    std::array<EnemyConfig, static_cast<std::size_t>(EnemyKind::Count)> enemies;
    std::array<ProjectileConfig, static_cast<std::size_t>(ProjectileKind::Count)> projectiles;
    PickupConfig pickups;
	MissileConfig missile;
	BossConfig boss;
	PartConfig parts;
    std::vector<LevelConfig> levels;
    float hitFlashDuration = 0.1f;
    EffectsConfig effects;
};
