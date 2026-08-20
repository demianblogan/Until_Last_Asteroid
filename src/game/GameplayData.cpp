#include "GameplayData.h"

#include <fstream>
#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace
{
    using Json = nlohmann::json;

    Json LoadJson(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("Failed to load gameplay data: " + path.string());

        try
        {
            return Json::parse(file);
        }
        catch (const Json::exception& exception)
        {
            throw std::runtime_error(
                "Invalid gameplay data " + path.string() + ": " + exception.what());
        }
    }

    template <typename Value>
    Value Require(const Json& object, const char* key, const std::filesystem::path& path)
    {
        try
        {
            return object.at(key).get<Value>();
        }
        catch (const Json::exception& exception)
        {
            throw std::runtime_error(
                "Invalid gameplay value '" + std::string(key) + "' in " +
                path.string() + ": " + exception.what());
        }
    }

    void RequirePositive(float value, const char* key, const std::filesystem::path& path)
    {
        if (value <= 0.f)
            throw std::runtime_error(
                "Gameplay value '" + std::string(key) + "' must be positive in " + path.string());
    }

    void RequireNonNegative(float value, const char* key, const std::filesystem::path& path)
    {
        if (value < 0.f)
            throw std::runtime_error(
                "Gameplay value '" + std::string(key) + "' cannot be negative in " + path.string());
    }

    GameplayData::EnemyKind ParseEnemyKind(
        const std::string& value,
        const std::filesystem::path& path)
    {
        if (value == "big_meteor")
            return GameplayData::EnemyKind::BigMeteor;
        if (value == "small_meteor")
            return GameplayData::EnemyKind::SmallMeteor;
        if (value == "kamikaze")
            return GameplayData::EnemyKind::Kamikaze;
        if (value == "shooter")
            return GameplayData::EnemyKind::Shooter;
        if (value == "spinner")
			return GameplayData::EnemyKind::Spinner;
		if (value == "missile_carrier")
			return GameplayData::EnemyKind::MissileCarrier;
		if (value == "laser_turret")
			return GameplayData::EnemyKind::LaserTurret;
		if (value == "shooter_station")
			return GameplayData::EnemyKind::ShooterStation;
		if (value == "reflector_gunship")
			return GameplayData::EnemyKind::ReflectorGunship;

        throw std::runtime_error(
            "Unknown gameplay enemy type '" + value + "' in " + path.string());
    }

	GameplayData::PickupKind ParsePickupKind(
		const std::string& value,
		const std::filesystem::path& path)
	{
		if (value == "health")
			return GameplayData::PickupKind::Health;
		if (value == "shield")
			return GameplayData::PickupKind::Shield;
		if (value == "homing_bullets")
			return GameplayData::PickupKind::HomingBullets;
		if (value == "time_slowdown")
			return GameplayData::PickupKind::TimeSlowdown;
		if (value == "laser")
			return GameplayData::PickupKind::Laser;
		if (value == "triple_shot")
			return GameplayData::PickupKind::TripleShot;
		if (value == "helper_bot")
			return GameplayData::PickupKind::HelperBot;

		throw std::runtime_error(
			"Unknown pickup type '" + value + "' in " + path.string());
	}

	GameplayData::SpawnGroup::PickupDropConfig ReadPickupDrop(
		const Json& object,
		const std::filesystem::path& path)
	{
		GameplayData::SpawnGroup::PickupDropConfig result;
		result.chance = object.contains("chance")
			? Require<float>(object, "chance", path)
			: 1.f;
		if (result.chance < 0.f || result.chance > 1.f)
			throw std::runtime_error(
				"Pickup drop chance must be in [0, 1] in " + path.string());

		const bool hasType{ object.contains("type") };
		const bool hasPool{ object.contains("pool") };
		if (hasType == hasPool)
			throw std::runtime_error(
				"Pickup drop must define exactly one of 'type' or 'pool' in " + path.string());

		if (hasType)
		{
			result.pool.push_back({ ParsePickupKind(
				Require<std::string>(object, "type", path), path), 1.f });
			return result;
		}

		const Json& pool{ object.at("pool") };
		if (!pool.is_array() || pool.empty())
			throw std::runtime_error(
				"Pickup drop pool must be a non-empty array in " + path.string());
		for (const Json& entry : pool)
		{
			GameplayData::SpawnGroup::PickupDropConfig::WeightedPickup pickup;
			pickup.kind = ParsePickupKind(
				Require<std::string>(entry, "type", path), path);
			pickup.weight = Require<float>(entry, "weight", path);
			RequirePositive(pickup.weight, "weight", path);
			result.pool.push_back(pickup);
		}
		return result;
	}

    std::vector<Collision::LocalCircle> ReadCollisionCircles(
        const Json& object,
        const std::filesystem::path& path)
    {
        if (!object.contains("collision_circles"))
            return {};

        const Json& circles{ object.at("collision_circles") };
        if (!circles.is_array() || circles.empty() || circles.size() > 8)
        {
            throw std::runtime_error(
                "Gameplay value 'collision_circles' must contain between one and eight circles in " +
                path.string());
        }

        std::vector<Collision::LocalCircle> result;
        result.reserve(circles.size());
        for (const Json& circle : circles)
        {
            Collision::LocalCircle value;
            value.offset.x = Require<float>(circle, "x", path);
            value.offset.y = Require<float>(circle, "y", path);
            value.radius = Require<float>(circle, "radius", path);
            if (!std::isfinite(value.offset.x) || !std::isfinite(value.offset.y))
                throw std::runtime_error(
                    "Collision-circle offsets must be finite in " + path.string());
            RequirePositive(value.radius, "radius", path);
            result.push_back(value);
        }
        return result;
    }

    GameplayData::EnemyConfig ReadEnemy(
        const Json& object,
        const std::filesystem::path& path)
    {
        GameplayData::EnemyConfig result;
        result.maximumHealth = Require<int>(object, "maximum_health", path);
        result.contactDamage = Require<int>(object, "contact_damage", path);
        result.score = Require<int>(object, "score", path);
        result.speed = Require<float>(object, "speed", path);
        result.collisionImpulse = Require<float>(object, "collision_impulse", path);
        result.actionInterval = Require<float>(object, "action_interval", path);
        result.fragmentSpeed = Require<float>(object, "fragment_speed", path);
        result.soundPitch = Require<float>(object, "sound_pitch", path);
        result.visualScale = Require<float>(object, "visual_scale", path);
        result.collisionRadius = Require<float>(object, "collision_radius", path);
		result.collisionCircles = ReadCollisionCircles(object, path);
		if (object.contains("rotation_speed"))
			result.rotationSpeed = Require<float>(object, "rotation_speed", path);
		if (object.contains("sine_amplitude"))
			result.sineAmplitude = Require<float>(object, "sine_amplitude", path);
		if (object.contains("sine_frequency"))
			result.sineFrequency = Require<float>(object, "sine_frequency", path);
		if (object.contains("beam_damage"))
			result.beamDamage = Require<int>(object, "beam_damage", path);
		if (object.contains("beam_width"))
			result.beamWidth = Require<float>(object, "beam_width", path);
		if (object.contains("shield_duration"))
			result.shieldDuration = Require<float>(object, "shield_duration", path);
		if (object.contains("spawn_animation_duration"))
			result.spawnAnimationDuration = Require<float>(
				object, "spawn_animation_duration", path);
		if (object.contains("weapon_emitters"))
		{
			const Json& emitters{ object.at("weapon_emitters") };
			if (!emitters.is_array() || emitters.empty())
				throw std::runtime_error(
					"Gameplay value 'weapon_emitters' must contain at least one point in " +
					path.string());

			result.weaponEmitters.reserve(emitters.size());
			for (const Json& emitterJson : emitters)
			{
				GameplayData::NormalizedPoint emitter;
				emitter.x = Require<float>(emitterJson, "x", path);
				emitter.y = Require<float>(emitterJson, "y", path);
				if (emitter.x < 0.f || emitter.x > 1.f ||
					emitter.y < 0.f || emitter.y > 1.f)
				{
					throw std::runtime_error(
						"Weapon emitter coordinates must be normalized to [0, 1] in " +
						path.string());
				}
				result.weaponEmitters.push_back(emitter);
			}
		}
		if (object.contains("engine_emitters"))
		{
			const Json& emitters{ object.at("engine_emitters") };
			if (!emitters.is_array() || emitters.empty())
				throw std::runtime_error(
					"Gameplay value 'engine_emitters' must contain at least one point in " +
					path.string());
			result.engineEmitters.reserve(emitters.size());
			for (const Json& emitterJson : emitters)
			{
				GameplayData::NormalizedPoint emitter;
				emitter.x = Require<float>(emitterJson, "x", path);
				emitter.y = Require<float>(emitterJson, "y", path);
				if (emitter.x < 0.f || emitter.x > 1.f ||
					emitter.y < 0.f || emitter.y > 1.f)
				{
					throw std::runtime_error(
						"Engine emitter coordinates must be normalized to [0, 1] in " +
						path.string());
				}
				result.engineEmitters.push_back(emitter);
			}
		}

        RequirePositive(static_cast<float>(result.maximumHealth), "maximum_health", path);
        RequireNonNegative(static_cast<float>(result.contactDamage), "contact_damage", path);
        RequireNonNegative(static_cast<float>(result.score), "score", path);
        RequirePositive(result.speed, "speed", path);
        RequireNonNegative(result.collisionImpulse, "collision_impulse", path);
        RequireNonNegative(result.actionInterval, "action_interval", path);
        RequireNonNegative(result.fragmentSpeed, "fragment_speed", path);
        RequirePositive(result.soundPitch, "sound_pitch", path);
        RequirePositive(result.visualScale, "visual_scale", path);
        RequirePositive(result.collisionRadius, "collision_radius", path);
		RequireNonNegative(result.rotationSpeed, "rotation_speed", path);
		RequireNonNegative(result.sineAmplitude, "sine_amplitude", path);
		RequireNonNegative(result.sineFrequency, "sine_frequency", path);
		RequireNonNegative(static_cast<float>(result.beamDamage), "beam_damage", path);
		RequireNonNegative(result.beamWidth, "beam_width", path);
		RequireNonNegative(result.shieldDuration, "shield_duration", path);
		RequireNonNegative(result.spawnAnimationDuration, "spawn_animation_duration", path);
        return result;
    }

    GameplayData::ProjectileConfig ReadProjectile(
        const Json& object,
        const std::filesystem::path& path)
    {
        GameplayData::ProjectileConfig result;
        result.damage = Require<int>(object, "damage", path);
        result.speed = Require<float>(object, "speed", path);
        result.knockback = Require<float>(object, "knockback", path);
        result.visualScale = Require<float>(object, "visual_scale", path);
        result.collisionRadius = Require<float>(object, "collision_radius", path);
        RequirePositive(static_cast<float>(result.damage), "damage", path);
        RequirePositive(result.speed, "speed", path);
        RequireNonNegative(result.knockback, "knockback", path);
        RequirePositive(result.visualScale, "visual_scale", path);
        RequirePositive(result.collisionRadius, "collision_radius", path);
        return result;
    }

    GameplayData::BurstConfig ReadBurst(
        const Json& object,
        const std::filesystem::path& path)
    {
        GameplayData::BurstConfig result;
        result.count = Require<int>(object, "count", path);
        result.minimumSpeed = Require<float>(object, "minimum_speed", path);
        result.maximumSpeed = Require<float>(object, "maximum_speed", path);
        result.minimumLifetime = Require<float>(object, "minimum_lifetime", path);
        result.maximumLifetime = Require<float>(object, "maximum_lifetime", path);
        result.minimumSize = Require<float>(object, "minimum_size", path);
        result.maximumSize = Require<float>(object, "maximum_size", path);

        RequirePositive(static_cast<float>(result.count), "count", path);
        RequireNonNegative(result.minimumSpeed, "minimum_speed", path);
        RequirePositive(result.maximumSpeed, "maximum_speed", path);
        RequirePositive(result.minimumLifetime, "minimum_lifetime", path);
        RequirePositive(result.maximumLifetime, "maximum_lifetime", path);
        RequirePositive(result.minimumSize, "minimum_size", path);
        RequirePositive(result.maximumSize, "maximum_size", path);
        if (result.maximumSpeed < result.minimumSpeed ||
            result.maximumLifetime < result.minimumLifetime ||
            result.maximumSize < result.minimumSize)
        {
            throw std::runtime_error(
                "Gameplay effect maximum values must not be below their minimums in " +
                path.string());
        }
        return result;
    }

    GameplayData::CameraShakeConfig ReadCameraShake(
        const Json& object,
        const std::filesystem::path& path)
    {
        GameplayData::CameraShakeConfig result;
        result.duration = Require<float>(object, "duration", path);
        result.amplitude = Require<float>(object, "amplitude", path);
        RequirePositive(result.duration, "duration", path);
        RequireNonNegative(result.amplitude, "amplitude", path);
        return result;
    }

    GameplayData::LevelConfig::PostProcessConfig ReadPostProcess(
        const Json& object,
        const std::filesystem::path& path)
    {
        GameplayData::LevelConfig::PostProcessConfig result;
        const Json& tint{ object.at("tint") };
        if (!tint.is_array() || tint.size() != result.tint.size())
            throw std::runtime_error(
                "Gameplay value 'tint' must contain exactly three values in " + path.string());
        for (std::size_t index{ 0u }; index < result.tint.size(); ++index)
            result.tint[index] = tint.at(index).get<float>();

        result.saturation = Require<float>(object, "saturation", path);
        result.contrast = Require<float>(object, "contrast", path);
        result.bloomIntensity = Require<float>(object, "bloom_intensity", path);
        result.vignetteStrength = Require<float>(object, "vignette_strength", path);

        for (float channel : result.tint)
            RequirePositive(channel, "tint", path);
        RequirePositive(result.saturation, "saturation", path);
        RequirePositive(result.contrast, "contrast", path);
        RequireNonNegative(result.bloomIntensity, "bloom_intensity", path);
        RequireNonNegative(result.vignetteStrength, "vignette_strength", path);
        if (result.vignetteStrength > 1.f)
            throw std::runtime_error(
                "Post-process vignette is outside its safe range in " + path.string());
        return result;
    }
}

GameplayData::GameplayData(const std::filesystem::path& directory)
{
    const std::filesystem::path playerPath{ directory / "player.json" };
    const Json playerJson{ LoadJson(playerPath) };
    player.maximumHealth = Require<int>(playerJson, "maximum_health", playerPath);
    player.collisionDamage = Require<int>(playerJson, "collision_damage", playerPath);
    player.damageInvulnerability = Require<float>(playerJson, "damage_invulnerability", playerPath);
    player.collisionImpulse = Require<float>(playerJson, "collision_impulse", playerPath);
    player.acceleration = Require<float>(playerJson, "acceleration", playerPath);
    player.damping = Require<float>(playerJson, "damping", playerPath);
    player.maximumSpeed = Require<float>(playerJson, "maximum_speed", playerPath);
    player.shootCooldown = Require<float>(playerJson, "shoot_cooldown", playerPath);
    player.visualScale = Require<float>(playerJson, "visual_scale", playerPath);
    player.collisionRadius = Require<float>(playerJson, "collision_radius", playerPath);
	player.collisionCircles = ReadCollisionCircles(playerJson, playerPath);
    try
    {
        const Json& emitters{ playerJson.at("engine_emitters") };
        if (!emitters.is_array() || emitters.size() != player.engineEmitters.size())
            throw std::runtime_error(
                "Gameplay value 'engine_emitters' must contain exactly two points in " +
                playerPath.string());

        for (std::size_t i{ 0 }; i < player.engineEmitters.size(); ++i)
        {
            player.engineEmitters[i].x = Require<float>(emitters.at(i), "x", playerPath);
            player.engineEmitters[i].y = Require<float>(emitters.at(i), "y", playerPath);
            if (player.engineEmitters[i].x < 0.f || player.engineEmitters[i].x > 1.f ||
                player.engineEmitters[i].y < 0.f || player.engineEmitters[i].y > 1.f)
            {
                throw std::runtime_error(
                    "Engine emitter coordinates must be normalized to [0, 1] in " +
                    playerPath.string());
            }
        }
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error(
            "Invalid engine emitters in " + playerPath.string() + ": " + exception.what());
    }
    try
    {
        const Json& muzzle{ playerJson.at("muzzle_emitter") };
        player.muzzleEmitter.x = Require<float>(muzzle, "x", playerPath);
        player.muzzleEmitter.y = Require<float>(muzzle, "y", playerPath);
        if (player.muzzleEmitter.x < 0.f || player.muzzleEmitter.x > 1.f ||
            player.muzzleEmitter.y < 0.f || player.muzzleEmitter.y > 1.f)
        {
            throw std::runtime_error(
                "Muzzle emitter coordinates must be normalized to [0, 1] in " +
                playerPath.string());
        }
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error(
            "Invalid muzzle emitter in " + playerPath.string() + ": " + exception.what());
    }
    hitFlashDuration = Require<float>(playerJson, "hit_flash_duration", playerPath);

    RequirePositive(static_cast<float>(player.maximumHealth), "maximum_health", playerPath);
    RequirePositive(static_cast<float>(player.collisionDamage), "collision_damage", playerPath);
    RequirePositive(player.damageInvulnerability, "damage_invulnerability", playerPath);
    RequireNonNegative(player.collisionImpulse, "collision_impulse", playerPath);
    RequirePositive(player.acceleration, "acceleration", playerPath);
    if (player.damping <= 0.f || player.damping > 1.f)
        throw std::runtime_error("Gameplay value 'damping' must be in (0, 1] in " + playerPath.string());
    RequirePositive(player.maximumSpeed, "maximum_speed", playerPath);
    RequirePositive(player.shootCooldown, "shoot_cooldown", playerPath);
    RequirePositive(player.visualScale, "visual_scale", playerPath);
    RequirePositive(player.collisionRadius, "collision_radius", playerPath);
    RequirePositive(hitFlashDuration, "hit_flash_duration", playerPath);

    const std::filesystem::path enemiesPath{ directory / "enemies.json" };
    const Json enemiesJson{ LoadJson(enemiesPath) };
    const std::array<std::pair<const char*, EnemyKind>, 9> enemyNames{
        std::pair{ "big_meteor", EnemyKind::BigMeteor },
        std::pair{ "small_meteor", EnemyKind::SmallMeteor },
        std::pair{ "kamikaze", EnemyKind::Kamikaze },
        std::pair{ "shooter", EnemyKind::Shooter },
		std::pair{ "spinner", EnemyKind::Spinner },
		std::pair{ "missile_carrier", EnemyKind::MissileCarrier },
		std::pair{ "laser_turret", EnemyKind::LaserTurret },
		std::pair{ "shooter_station", EnemyKind::ShooterStation },
		std::pair{ "reflector_gunship", EnemyKind::ReflectorGunship }
    };
    for (const auto& [name, kind] : enemyNames)
    {
        try
        {
			const Json& enemyJson{ enemiesJson.at(name) };
			if (kind == EnemyKind::Kamikaze && !enemyJson.contains("rotation_speed"))
				throw std::runtime_error(
					"Enemy 'kamikaze' requires 'rotation_speed' in " + enemiesPath.string());
			if ((kind == EnemyKind::Shooter || kind == EnemyKind::Spinner ||
				kind == EnemyKind::MissileCarrier) &&
				!enemyJson.contains("weapon_emitters"))
				throw std::runtime_error(
					"Armed enemy requires 'weapon_emitters' in " + enemiesPath.string());
			EnemyConfig config{ ReadEnemy(enemyJson, enemiesPath) };
			if (kind == EnemyKind::Kamikaze)
				RequirePositive(config.rotationSpeed, "rotation_speed", enemiesPath);
			if (kind == EnemyKind::Shooter ||
				kind == EnemyKind::MissileCarrier ||
				kind == EnemyKind::ReflectorGunship)
			{
				if (!enemyJson.contains("rotation_speed"))
					throw std::runtime_error(
						"Aiming enemy requires 'rotation_speed' in " +
						enemiesPath.string());
				RequirePositive(config.rotationSpeed, "rotation_speed", enemiesPath);
			}
			if (kind == EnemyKind::Shooter || kind == EnemyKind::Spinner ||
				kind == EnemyKind::MissileCarrier)
				RequirePositive(config.actionInterval, "action_interval", enemiesPath);
			if (kind == EnemyKind::Shooter && config.weaponEmitters.size() != 2)
				throw std::runtime_error(
					"Enemy 'shooter' requires exactly two weapon emitters in " +
					enemiesPath.string());
			if (kind == EnemyKind::Spinner)
			{
				if (config.weaponEmitters.size() != 3)
					throw std::runtime_error(
						"Enemy 'spinner' requires exactly three weapon emitters in " +
						enemiesPath.string());
				RequirePositive(config.rotationSpeed, "rotation_speed", enemiesPath);
				RequirePositive(config.sineAmplitude, "sine_amplitude", enemiesPath);
				RequirePositive(config.sineFrequency, "sine_frequency", enemiesPath);
			}
			if (kind == EnemyKind::MissileCarrier && config.weaponEmitters.size() != 1)
				throw std::runtime_error(
					"Enemy 'missile_carrier' requires exactly one weapon emitter in " +
					enemiesPath.string());
			if (kind == EnemyKind::MissileCarrier && config.engineEmitters.size() != 2)
				throw std::runtime_error(
					"Enemy 'missile_carrier' requires exactly two engine emitters in " +
					enemiesPath.string());
			if (kind == EnemyKind::LaserTurret)
			{
				RequirePositive(static_cast<float>(config.beamDamage), "beam_damage", enemiesPath);
				RequirePositive(config.beamWidth, "beam_width", enemiesPath);
			}
			if (kind == EnemyKind::ShooterStation)
			{
				RequirePositive(config.actionInterval, "action_interval", enemiesPath);
				RequirePositive(config.shieldDuration, "shield_duration", enemiesPath);
				RequirePositive(config.spawnAnimationDuration,
					"spawn_animation_duration", enemiesPath);
			}
			if (kind == EnemyKind::ReflectorGunship)
			{
				RequirePositive(config.actionInterval, "action_interval", enemiesPath);
				RequirePositive(config.shieldDuration, "shield_duration", enemiesPath);
				RequirePositive(config.sineAmplitude, "sine_amplitude", enemiesPath);
				RequirePositive(config.sineFrequency, "sine_frequency", enemiesPath);
				if (config.weaponEmitters.size() != 2)
					throw std::runtime_error(
						"Enemy 'reflector_gunship' requires exactly two weapon emitters in " +
						enemiesPath.string());
			}
			enemies[static_cast<std::size_t>(kind)] = config;
        }
        catch (const Json::exception& exception)
        {
            throw std::runtime_error(
                "Invalid enemy '" + std::string(name) + "' in " + enemiesPath.string() +
                ": " + exception.what());
        }
    }

    const std::filesystem::path weaponsPath{ directory / "weapons.json" };
    const Json weaponsJson{ LoadJson(weaponsPath) };
    try
    {
        projectiles[static_cast<std::size_t>(ProjectileKind::Player)] =
            ReadProjectile(weaponsJson.at("player_shot"), weaponsPath);
		projectiles[static_cast<std::size_t>(ProjectileKind::Helper)] =
			ReadProjectile(weaponsJson.at("helper_bot_shot"), weaponsPath);
        projectiles[static_cast<std::size_t>(ProjectileKind::Enemy)] =
            ReadProjectile(weaponsJson.at("enemy_shot"), weaponsPath);
		projectiles[static_cast<std::size_t>(ProjectileKind::Spinner)] =
			ReadProjectile(weaponsJson.at("spinner_shot"), weaponsPath);

		const Json& missileJson{ weaponsJson.at("homing_missile") };
		missile.maximumHealth = Require<int>(missileJson, "maximum_health", weaponsPath);
		missile.explosionDamage = Require<int>(missileJson, "explosion_damage", weaponsPath);
		missile.speed = Require<float>(missileJson, "speed", weaponsPath);
		missile.turnSpeedDegrees = Require<float>(missileJson, "turn_speed_degrees", weaponsPath);
		missile.explosionRadius = Require<float>(missileJson, "explosion_radius", weaponsPath);
		missile.explosionImpulse = Require<float>(missileJson, "explosion_impulse", weaponsPath);
		missile.lifetime = Require<float>(missileJson, "lifetime", weaponsPath);
		missile.visualScale = Require<float>(missileJson, "visual_scale", weaponsPath);
		missile.collisionRadius = Require<float>(missileJson, "collision_radius", weaponsPath);
		RequirePositive(static_cast<float>(missile.maximumHealth), "maximum_health", weaponsPath);
		RequirePositive(static_cast<float>(missile.explosionDamage), "explosion_damage", weaponsPath);
		RequirePositive(missile.speed, "speed", weaponsPath);
		RequirePositive(missile.turnSpeedDegrees, "turn_speed_degrees", weaponsPath);
		RequirePositive(missile.explosionRadius, "explosion_radius", weaponsPath);
		RequireNonNegative(missile.explosionImpulse, "explosion_impulse", weaponsPath);
		RequirePositive(missile.lifetime, "lifetime", weaponsPath);
		RequirePositive(missile.visualScale, "visual_scale", weaponsPath);
		RequirePositive(missile.collisionRadius, "collision_radius", weaponsPath);
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error(
            "Invalid projectile data in " + weaponsPath.string() + ": " + exception.what());
    }

    const std::filesystem::path effectsPath{ directory / "effects.json" };
    const Json effectsJson{ LoadJson(effectsPath) };
    try
    {
        effects.stoneHit = ReadBurst(effectsJson.at("stone_hit"), effectsPath);
        effects.metalHit = ReadBurst(effectsJson.at("metal_hit"), effectsPath);
        effects.smallAsteroidExplosion = ReadBurst(
            effectsJson.at("small_asteroid_explosion"), effectsPath);
        effects.largeAsteroidExplosion = ReadBurst(
            effectsJson.at("large_asteroid_explosion"), effectsPath);
        effects.shipExplosion = ReadBurst(effectsJson.at("ship_explosion"), effectsPath);
        effects.damageShake = ReadCameraShake(effectsJson.at("damage_shake"), effectsPath);
        effects.largeExplosionShake = ReadCameraShake(
            effectsJson.at("large_explosion_shake"), effectsPath);
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error(
            "Invalid visual effect data in " + effectsPath.string() + ": " + exception.what());
    }

    const std::filesystem::path pickupsPath{ directory / "pickups.json" };
    const Json pickupsJson{ LoadJson(pickupsPath) };
    pickups.healthRestorePercentage = Require<float>(
        pickupsJson, "health_restore_percentage", pickupsPath);
    pickups.shieldCapacity = Require<float>(pickupsJson, "shield_capacity", pickupsPath);
    pickups.shieldDuration = Require<float>(pickupsJson, "shield_duration", pickupsPath);
	pickups.homingBulletsDuration = Require<float>(
		pickupsJson, "homing_bullets_duration", pickupsPath);
	pickups.homingConeDegrees = Require<float>(
		pickupsJson, "homing_cone_degrees", pickupsPath);
	pickups.homingTurnSpeedDegrees = Require<float>(
		pickupsJson, "homing_turn_speed_degrees", pickupsPath);
	pickups.laserDuration = Require<float>(
		pickupsJson, "laser_duration", pickupsPath);
	pickups.laserDamageInterval = Require<float>(
		pickupsJson, "laser_damage_interval", pickupsPath);
	pickups.laserWidth = Require<float>(
		pickupsJson, "laser_width", pickupsPath);
	pickups.tripleShotDuration = Require<float>(
		pickupsJson, "triple_shot_duration", pickupsPath);
	pickups.tripleShotAngleDegrees = Require<float>(
		pickupsJson, "triple_shot_angle_degrees", pickupsPath);
	pickups.timeSlowdownDuration = Require<float>(
		pickupsJson, "time_slowdown_duration", pickupsPath);
	pickups.timeSlowdownWorldScale = Require<float>(
		pickupsJson, "time_slowdown_world_scale", pickupsPath);
	pickups.timeSlowdownAudioPitch = Require<float>(
		pickupsJson, "time_slowdown_audio_pitch", pickupsPath);
	pickups.helperBotShotInterval = Require<float>(
		pickupsJson, "helper_bot_shot_interval", pickupsPath);
	pickups.helperBotTurnSpeedDegrees = Require<float>(
		pickupsJson, "helper_bot_turn_speed_degrees", pickupsPath);
	pickups.helperBotOrbitRadius = Require<float>(
		pickupsJson, "helper_bot_orbit_radius", pickupsPath);
	pickups.helperBotOrbitSpeedDegrees = Require<float>(
		pickupsJson, "helper_bot_orbit_speed_degrees", pickupsPath);
	pickups.helperBotVisualScale = Require<float>(
		pickupsJson, "helper_bot_visual_scale", pickupsPath);
    pickups.visualScale = Require<float>(pickupsJson, "visual_scale", pickupsPath);
    pickups.collisionRadius = Require<float>(pickupsJson, "collision_radius", pickupsPath);
    RequirePositive(pickups.healthRestorePercentage, "health_restore_percentage", pickupsPath);
    if (pickups.healthRestorePercentage > 1.f)
        throw std::runtime_error("Health restore percentage must not exceed 1 in " + pickupsPath.string());
    RequirePositive(pickups.shieldCapacity, "shield_capacity", pickupsPath);
    RequirePositive(pickups.shieldDuration, "shield_duration", pickupsPath);
	RequirePositive(pickups.homingBulletsDuration, "homing_bullets_duration", pickupsPath);
	RequirePositive(pickups.homingConeDegrees, "homing_cone_degrees", pickupsPath);
	if (pickups.homingConeDegrees > 360.f)
		throw std::runtime_error(
			"Homing cone must not exceed 360 degrees in " + pickupsPath.string());
	RequirePositive(pickups.homingTurnSpeedDegrees, "homing_turn_speed_degrees", pickupsPath);
	RequirePositive(pickups.laserDuration, "laser_duration", pickupsPath);
	RequirePositive(pickups.laserDamageInterval, "laser_damage_interval", pickupsPath);
	RequirePositive(pickups.laserWidth, "laser_width", pickupsPath);
	RequirePositive(pickups.tripleShotDuration, "triple_shot_duration", pickupsPath);
	RequirePositive(pickups.tripleShotAngleDegrees, "triple_shot_angle_degrees", pickupsPath);
	if (pickups.tripleShotAngleDegrees >= 45.f)
		throw std::runtime_error(
			"Triple-shot angle must be below 45 degrees in " + pickupsPath.string());
	RequirePositive(pickups.timeSlowdownDuration, "time_slowdown_duration", pickupsPath);
	RequirePositive(pickups.timeSlowdownWorldScale, "time_slowdown_world_scale", pickupsPath);
	if (pickups.timeSlowdownWorldScale > 1.f)
		throw std::runtime_error(
			"Time slowdown world scale must not exceed 1 in " + pickupsPath.string());
	RequirePositive(pickups.timeSlowdownAudioPitch, "time_slowdown_audio_pitch", pickupsPath);
	if (pickups.timeSlowdownAudioPitch > 1.f)
		throw std::runtime_error(
			"Time slowdown audio pitch must not exceed 1 in " + pickupsPath.string());
	RequirePositive(pickups.helperBotShotInterval, "helper_bot_shot_interval", pickupsPath);
	RequirePositive(pickups.helperBotTurnSpeedDegrees, "helper_bot_turn_speed_degrees", pickupsPath);
	RequirePositive(pickups.helperBotOrbitRadius, "helper_bot_orbit_radius", pickupsPath);
	RequirePositive(pickups.helperBotOrbitSpeedDegrees, "helper_bot_orbit_speed_degrees", pickupsPath);
	RequirePositive(pickups.helperBotVisualScale, "helper_bot_visual_scale", pickupsPath);
    RequirePositive(pickups.visualScale, "visual_scale", pickupsPath);
    RequirePositive(pickups.collisionRadius, "collision_radius", pickupsPath);
	parts.lifetime = Require<float>(pickupsJson, "part_lifetime", pickupsPath);
	parts.blinkDuration = Require<float>(pickupsJson, "part_blink_duration", pickupsPath);
	parts.visualScale = Require<float>(pickupsJson, "part_visual_scale", pickupsPath);
	parts.collisionRadius = Require<float>(pickupsJson, "part_collision_radius", pickupsPath);
	parts.rotationSpeedDegrees = Require<float>(
		pickupsJson, "part_rotation_speed_degrees", pickupsPath);
	RequirePositive(parts.lifetime, "part_lifetime", pickupsPath);
	RequirePositive(parts.blinkDuration, "part_blink_duration", pickupsPath);
	if (parts.blinkDuration > parts.lifetime)
		throw std::runtime_error("Part blink duration must not exceed lifetime in " + pickupsPath.string());
	RequirePositive(parts.visualScale, "part_visual_scale", pickupsPath);
	RequirePositive(parts.collisionRadius, "part_collision_radius", pickupsPath);
	RequireNonNegative(parts.rotationSpeedDegrees, "part_rotation_speed_degrees", pickupsPath);

    const std::filesystem::path levelsPath{ directory / "levels.json" };
    const Json levelsJson{ LoadJson(levelsPath) };
    const Json* levelArray{ nullptr };
    try
    {
        levelArray = &levelsJson.at("levels");
        if (!levelArray->is_array() || levelArray->empty())
            throw std::runtime_error("Gameplay levels must be a non-empty array in " + levelsPath.string());
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error(
            "Invalid levels in " + levelsPath.string() + ": " + exception.what());
    }

    std::unordered_set<std::string> knownPartIds;
    for (const Json& levelJson : *levelArray)
    {
        LevelConfig level;
        level.number = Require<int>(levelJson, "number", levelsPath);
		level.title = Require<std::string>(levelJson, "title", levelsPath);
        level.background = Require<std::string>(levelJson, "background", levelsPath);
        level.backgroundBrightness = Require<float>(
            levelJson, "background_brightness", levelsPath);
		level.targetAccuracyPercent = Require<float>(
			levelJson, "target_accuracy_percent", levelsPath);
        RequirePositive(level.backgroundBrightness, "background_brightness", levelsPath);
		RequirePositive(level.targetAccuracyPercent, "target_accuracy_percent", levelsPath);
		if (level.targetAccuracyPercent > 100.f)
			throw std::runtime_error(
				"Target accuracy must not exceed 100 percent in " + levelsPath.string());
        if (level.backgroundBrightness > 1.f)
            throw std::runtime_error(
                "Level background brightness must not exceed 1 in " + levelsPath.string());
        try
        {
            level.postProcess = ReadPostProcess(levelJson.at("post_process"), levelsPath);
        }
        catch (const Json::exception& exception)
        {
            throw std::runtime_error(
                "Invalid post-process settings for level " + std::to_string(level.number) +
                " in " + levelsPath.string() + ": " + exception.what());
        }
        if (level.number != static_cast<int>(levels.size()) + 1)
            throw std::runtime_error("Gameplay levels must be sequential in " + levelsPath.string());
        if (level.background.empty())
            throw std::runtime_error("Level background cannot be empty in " + levelsPath.string());
		if (level.title.empty())
			throw std::runtime_error("Level title cannot be empty in " + levelsPath.string());

        try
        {
            for (const Json& waveJson : levelJson.at("waves"))
            {
                WaveConfig wave;
                for (const Json& spawnJson : waveJson.at("initial_spawns"))
                {
                    SpawnGroup spawn;
                    spawn.kind = ParseEnemyKind(
                        Require<std::string>(spawnJson, "type", levelsPath), levelsPath);
                    spawn.count = Require<int>(spawnJson, "count", levelsPath);
					if (spawnJson.contains("drop"))
						spawn.drop = ReadPickupDrop(spawnJson.at("drop"), levelsPath);
					spawn.partIds = spawnJson.value("part_ids", std::vector<std::string>{});
                    if (spawn.count <= 0)
                        throw std::runtime_error(
                            "Wave initial spawn count must be positive in " + levelsPath.string());
					if (spawn.partIds.size() > static_cast<std::size_t>(spawn.count))
						throw std::runtime_error("Part ID count exceeds enemy count in " + levelsPath.string());
					for (const std::string& id : spawn.partIds)
					{
						if (id.empty() || !knownPartIds.insert(id).second)
							throw std::runtime_error("Part IDs must be non-empty and unique in " + levelsPath.string());
						level.partIds.push_back(id);
					}
                    wave.initialSpawns.push_back(std::move(spawn));
                }

                for (const Json& spawnJson : waveJson.at("scheduled_spawns"))
                {
                    WaveConfig::ScheduledSpawn spawn;
                    spawn.delay = Require<float>(spawnJson, "delay", levelsPath);
                    spawn.kind = ParseEnemyKind(
                        Require<std::string>(spawnJson, "type", levelsPath), levelsPath);
                    spawn.count = Require<int>(spawnJson, "count", levelsPath);
					if (spawnJson.contains("drop"))
						spawn.drop = ReadPickupDrop(spawnJson.at("drop"), levelsPath);
					spawn.partIds = spawnJson.value("part_ids", std::vector<std::string>{});
                    RequireNonNegative(spawn.delay, "delay", levelsPath);
                    if (spawn.count <= 0)
                        throw std::runtime_error(
                            "Scheduled spawn count must be positive in " + levelsPath.string());
					if (spawn.partIds.size() > static_cast<std::size_t>(spawn.count))
						throw std::runtime_error("Part ID count exceeds enemy count in " + levelsPath.string());
					for (const std::string& id : spawn.partIds)
					{
						if (id.empty() || !knownPartIds.insert(id).second)
							throw std::runtime_error("Part IDs must be non-empty and unique in " + levelsPath.string());
						level.partIds.push_back(id);
					}
                    wave.scheduledSpawns.push_back(std::move(spawn));
                }

                if (wave.initialSpawns.empty() && wave.scheduledSpawns.empty())
                    throw std::runtime_error("Gameplay waves cannot be empty in " + levelsPath.string());
                level.waves.push_back(std::move(wave));
            }

            if (level.waves.empty())
                throw std::runtime_error("Gameplay levels must contain at least one wave in " + levelsPath.string());
			if (level.number != 10 && level.waves.size() != 3u)
				throw std::runtime_error(
					"Campaign levels must contain exactly three waves except Level 10 in " +
					levelsPath.string());
        }
        catch (const Json::exception& exception)
        {
            throw std::runtime_error(
                "Invalid level " + std::to_string(level.number) + " in " + levelsPath.string() +
                ": " + exception.what());
        }

        levels.push_back(std::move(level));
    }
}

const GameplayData::PlayerConfig& GameplayData::GetPlayer() const noexcept
{
    return player;
}

const GameplayData::EnemyConfig& GameplayData::GetEnemy(EnemyKind kind) const noexcept
{
    return enemies[static_cast<std::size_t>(kind)];
}

const GameplayData::ProjectileConfig& GameplayData::GetProjectile(ProjectileKind kind) const noexcept
{
    return projectiles[static_cast<std::size_t>(kind)];
}

const GameplayData::PickupConfig& GameplayData::GetPickups() const noexcept
{
    return pickups;
}

const GameplayData::MissileConfig& GameplayData::GetMissile() const noexcept
{
	return missile;
}

const GameplayData::PartConfig& GameplayData::GetParts() const noexcept
{
	return parts;
}

const GameplayData::LevelConfig& GameplayData::GetLevel(int number) const
{
    if (number <= 0 || number > static_cast<int>(levels.size()))
        throw std::out_of_range("Gameplay level is out of range: " + std::to_string(number));
    return levels[static_cast<std::size_t>(number - 1)];
}

int GameplayData::GetLevelCount() const noexcept
{
    return static_cast<int>(levels.size());
}

float GameplayData::GetHitFlashDuration() const noexcept
{
    return hitFlashDuration;
}

const GameplayData::EffectsConfig& GameplayData::GetEffects() const noexcept
{
    return effects;
}
