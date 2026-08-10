#include "GameplayData.h"

#include <fstream>
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
        if (value == "medium_meteor")
            return GameplayData::EnemyKind::MediumMeteor;
        if (value == "small_meteor")
            return GameplayData::EnemyKind::SmallMeteor;
        if (value == "kamikaze")
            return GameplayData::EnemyKind::Kamikaze;
        if (value == "shooter")
            return GameplayData::EnemyKind::Shooter;

        throw std::runtime_error(
            "Unknown gameplay enemy type '" + value + "' in " + path.string());
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

        RequirePositive(static_cast<float>(result.maximumHealth), "maximum_health", path);
        RequireNonNegative(static_cast<float>(result.contactDamage), "contact_damage", path);
        RequireNonNegative(static_cast<float>(result.score), "score", path);
        RequirePositive(result.speed, "speed", path);
        RequireNonNegative(result.collisionImpulse, "collision_impulse", path);
        RequireNonNegative(result.actionInterval, "action_interval", path);
        RequireNonNegative(result.fragmentSpeed, "fragment_speed", path);
        RequirePositive(result.soundPitch, "sound_pitch", path);
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
        RequirePositive(static_cast<float>(result.damage), "damage", path);
        RequirePositive(result.speed, "speed", path);
        RequireNonNegative(result.knockback, "knockback", path);
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
    RequirePositive(hitFlashDuration, "hit_flash_duration", playerPath);

    const std::filesystem::path enemiesPath{ directory / "enemies.json" };
    const Json enemiesJson{ LoadJson(enemiesPath) };
    const std::array<std::pair<const char*, EnemyKind>, 5> enemyNames{
        std::pair{ "big_meteor", EnemyKind::BigMeteor },
        std::pair{ "medium_meteor", EnemyKind::MediumMeteor },
        std::pair{ "small_meteor", EnemyKind::SmallMeteor },
        std::pair{ "kamikaze", EnemyKind::Kamikaze },
        std::pair{ "shooter", EnemyKind::Shooter }
    };
    for (const auto& [name, kind] : enemyNames)
    {
        try
        {
            enemies[static_cast<std::size_t>(kind)] = ReadEnemy(enemiesJson.at(name), enemiesPath);
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
        if (level.number != static_cast<int>(levels.size()) + 1)
            throw std::runtime_error("Gameplay levels must be sequential in " + levelsPath.string());

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
