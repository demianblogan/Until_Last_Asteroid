#pragma once

#include <optional>
#include <vector>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/World.h"
#include "game/GameplaySession.h"
#include "states/State.h"
#include "systems/ActionMap.h"
#include "systems/InputHandler.h"
#include "ui/HUD.h"
#include "utils/ConfigEnums.h"

class GameplayState final : public State
{
public:
    GameplayState(StateStack& stateStack, StateContext context);
    ~GameplayState() override;

    void HandleEvent(const sf::Event& event) override;
    void HandleRealtime() override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    struct SpawnWave
    {
        float interval;
        int totalSpawns;
        float timer{ 0.f };
        int spawned{ 0 };

        bool spawnKamikaze{ false };
        bool spawnShooter{ false };
    };

    struct LevelData
    {
        int initialMeteors{ 0 };
        int initialShooters{ 0 };

        std::vector<SpawnWave> waves;
    };

    void SetupInput();
    void SetupUI();
    void OpenPauseMenu();
    void ResumeGameplaySounds();

    void SpawnPlayerIfNeeded();

    void Reset();
    void NextLevel();
    void SpawnLevel();
    [[nodiscard]] LevelData CreateLevel(int level);

    void CenterTextX(sf::Text& text);
    void CenterText(sf::Text& text, float y);

    [[nodiscard]] sf::Vector2f GetSafeSpawnPosition();
    [[nodiscard]] sf::Vector2f GetSafeEdgeSpawnPosition();

    static constexpr float SPAWN_SAFE_RADIUS{ 250.f };

    GameplaySession session;
    ActionMap<Config::PlayerAction> actions;
    InputHandler<Config::PlayerAction> input;
    World world;
    std::optional<HUD> hud;

    std::vector<sf::Text> gameOverTexts;
    std::vector<sf::Text> levelCompleteTexts;
    std::vector<sf::Text> winTexts;
    std::optional<sf::Text> exitHintText;

    LevelData currentLevel;
    bool gameplaySoundsPaused{ false };
};
