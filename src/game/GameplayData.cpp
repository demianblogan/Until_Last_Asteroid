#include "GameplayData.h"

#include <fstream>
#include <cmath>
#include <stdexcept>
#include <string>

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

        throw std::runtime_error(
            "Unknown gameplay enemy type '" + value + "' in " + path.string());
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
		if (object.contains("weapon_emitters"))
		{
			const Json& emitters{ object.at("weapon_emitters") };
			if (!emitters.is_array() || emitters.size() != result.weaponEmitters.size())
				throw std::runtime_error(
					"Gameplay value 'weapon_emitters' must contain exactly two points in " +
					path.string());

			for (std::size_t i{ 0 }; i < result.weaponEmitters.size(); ++i)
			{
				result.weaponEmitters[i].x = Require<float>(emitters.at(i), "x", path);
				result.weaponEmitters[i].y = Require<float>(emitters.at(i), "y", path);
				if (result.weaponEmitters[i].x < 0.f || result.weaponEmitters[i].x > 1.f ||
					result.weaponEmitters[i].y < 0.f || result.weaponEmitters[i].y > 1.f)
				{
					throw std::runtime_error(
						"Weapon emitter coordinates must be normalized to [0, 1] in " +
						path.string());
				}
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
    const std::array<std::pair<const char*, EnemyKind>, 4> enemyNames{
        std::pair{ "big_meteor", EnemyKind::BigMeteor },
        std::pair{ "small_meteor", EnemyKind::SmallMeteor },
        std::pair{ "kamikaze", EnemyKind::Kamikaze },
        std::pair{ "shooter", EnemyKind::Shooter }
    };
    for (const auto& [name, kind] : enemyNames)
    {
        try
        {
			const Json& enemyJson{ enemiesJson.at(name) };
			if (kind == EnemyKind::Kamikaze && !enemyJson.contains("rotation_speed"))
				throw std::runtime_error(
					"Enemy 'kamikaze' requires 'rotation_speed' in " + enemiesPath.string());
			if (kind == EnemyKind::Shooter && !enemyJson.contains("weapon_emitters"))
				throw std::runtime_error(
					"Enemy 'shooter' requires 'weapon_emitters' in " + enemiesPath.string());
			EnemyConfig config{ ReadEnemy(enemyJson, enemiesPath) };
			if (kind == EnemyKind::Kamikaze)
				RequirePositive(config.rotationSpeed, "rotation_speed", enemiesPath);
			if (kind == EnemyKind::Shooter)
				RequirePositive(config.actionInterval, "action_interval", enemiesPath);
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
        projectiles[static_cast<std::size_t>(ProjectileKind::Enemy)] =
            ReadProjectile(weaponsJson.at("enemy_shot"), weaponsPath);
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

    for (const Json& levelJson : *levelArray)
    {
        LevelConfig level;
        level.number = Require<int>(levelJson, "number", levelsPath);
        level.background = Require<std::string>(levelJson, "background", levelsPath);
        level.backgroundBrightness = Require<float>(
            levelJson, "background_brightness", levelsPath);
        RequirePositive(level.backgroundBrightness, "background_brightness", levelsPath);
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

        try
        {
            for (const Json& spawnJson : levelJson.at("initial_spawns"))
            {
                SpawnGroup spawn;
                spawn.kind = ParseEnemyKind(Require<std::string>(spawnJson, "type", levelsPath), levelsPath);
                spawn.count = Require<int>(spawnJson, "count", levelsPath);
                if (spawn.count < 0)
                    throw std::runtime_error("Initial spawn count cannot be negative in " + levelsPath.string());
                level.initialSpawns.push_back(spawn);
            }

            for (const Json& waveJson : levelJson.at("waves"))
            {
                WaveConfig wave;
                wave.interval = Require<float>(waveJson, "interval", levelsPath);
                wave.repetitions = Require<int>(waveJson, "repetitions", levelsPath);
                RequirePositive(wave.interval, "interval", levelsPath);
                if (wave.repetitions <= 0)
                    throw std::runtime_error("Wave repetitions must be positive in " + levelsPath.string());

                for (const Json& spawn : waveJson.at("spawns"))
                    wave.spawns.push_back(ParseEnemyKind(spawn.get<std::string>(), levelsPath));
                if (wave.spawns.empty())
                    throw std::runtime_error("Wave spawns cannot be empty in " + levelsPath.string());
                level.waves.push_back(std::move(wave));
            }
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
