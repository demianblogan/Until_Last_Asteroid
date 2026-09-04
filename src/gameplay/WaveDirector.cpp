#include "WaveDirector.h"

void WaveDirector::LoadLevel(const GameplayData::LevelConfig& level)
{
    waves = &level.waves;
    nextWaveIndex = 0u;
    currentWaveIndex = 0u;
    nextScheduledSpawn = 0u;
    timeUntilNextSpawn = 0.f;
    isWaveActive = false;
}

bool WaveDirector::StartNextWave(const SpawnEnemy& spawnEnemy)
{
    if (waves == nullptr || nextWaveIndex >= waves->size())
        return false;

    currentWaveIndex = nextWaveIndex++;
    nextScheduledSpawn = 0u;
    isWaveActive = true;

    const GameplayData::WaveConfig& wave{ waves->at(currentWaveIndex) };
    for (const GameplayData::SpawnGroup& group : wave.initialSpawns)
        SpawnGroup(group, spawnEnemy);

    timeUntilNextSpawn = wave.scheduledSpawns.empty()
        ? 0.f
        : wave.scheduledSpawns.front().delay;
    return true;
}

void WaveDirector::Update(float deltaTime, const SpawnEnemy& spawnEnemy)
{
    if (!isWaveActive || waves == nullptr)
        return;

    const GameplayData::WaveConfig& wave{ waves->at(currentWaveIndex) };
    if (nextScheduledSpawn >= wave.scheduledSpawns.size())
        return;

    timeUntilNextSpawn -= deltaTime;
    while (nextScheduledSpawn < wave.scheduledSpawns.size() && timeUntilNextSpawn <= 0.f)
    {
        SpawnGroup(wave.scheduledSpawns[nextScheduledSpawn], spawnEnemy);
        ++nextScheduledSpawn;
        if (nextScheduledSpawn < wave.scheduledSpawns.size())
            timeUntilNextSpawn += wave.scheduledSpawns[nextScheduledSpawn].delay;
    }
}

bool WaveDirector::HasActiveWave() const noexcept
{
    return isWaveActive;
}

bool WaveDirector::HasMoreWaves() const noexcept
{
    return waves != nullptr && nextWaveIndex < waves->size();
}

bool WaveDirector::IsDeploymentComplete() const noexcept
{
    if (!isWaveActive || waves == nullptr)
        return false;
    return nextScheduledSpawn >= waves->at(currentWaveIndex).scheduledSpawns.size();
}

int WaveDirector::GetCurrentWaveNumber() const noexcept
{
    return isWaveActive ? static_cast<int>(currentWaveIndex + 1u) : 0;
}

int WaveDirector::GetWaveCount() const noexcept
{
    return waves == nullptr ? 0 : static_cast<int>(waves->size());
}

void WaveDirector::SpawnGroup(
    const GameplayData::SpawnGroup& group,
    const SpawnEnemy& spawnEnemy)
{
    for (int count{ 0 }; count < group.count; ++count)
        spawnEnemy(group, static_cast<std::size_t>(count));
}
