#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "game/GameplayData.h"

class WaveDirector
{
public:
    using SpawnEnemy = std::function<void(GameplayData::EnemyKind)>;

    void LoadLevel(const GameplayData::LevelConfig& level);
    bool StartNextWave(const SpawnEnemy& spawnEnemy);
    void Update(float deltaTime, const SpawnEnemy& spawnEnemy);

    [[nodiscard]] bool HasActiveWave() const noexcept;
    [[nodiscard]] bool HasMoreWaves() const noexcept;
    [[nodiscard]] bool IsDeploymentComplete() const noexcept;
    [[nodiscard]] int GetCurrentWaveNumber() const noexcept;
    [[nodiscard]] int GetWaveCount() const noexcept;

private:
    static void SpawnGroup(const GameplayData::SpawnGroup& group, const SpawnEnemy& spawnEnemy);

    const std::vector<GameplayData::WaveConfig>* waves{ nullptr };
    std::size_t nextWaveIndex{ 0u };
    std::size_t currentWaveIndex{ 0u };
    std::size_t nextScheduledSpawn{ 0u };
    float timeUntilNextSpawn{ 0.f };
    bool activeWave{ false };
};
