#pragma once

#include <optional>
#include <vector>
#include <SFML/Graphics/RenderWindow.hpp>

#include "assets/AssetStore.h"
#include "core/GameState.h"
#include "core/World.h"
#include "systems/ActionMap.h"
#include "systems/InputHandler.h"
#include "utils/ConfigEnums.h"
#include "ui/HUD.h"

class HUD;

namespace sf
{
	class RenderWindow;
	class Text;
	class View;
}

class Game
{
public:
	Game();
	void Run();

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

	void ProcessEvents();
	void Update(float dt);
	void Render();

	void SpawnPlayerIfNeeded();

	void Reset();
	void NextLevel();
	void SpawnLevel();
	LevelData CreateLevel(int level);

	void CenterTextX(sf::Text& text);
	void CenterText(sf::Text& text, float y);

	sf::Vector2f GetSafeSpawnPosition();
	sf::Vector2f GetSafeEdgeSpawnPosition();
	sf::View GetLetterboxView(const sf::View& view, int windowWidth, int windowHeight);

	static constexpr sf::Vector2f LOGICAL_SIZE{ 1920.f, 1080.f };
	static constexpr float SPAWN_SAFE_RADIUS = 250.f;
	static constexpr float MAX_FRAME_TIME = 0.1f;

	sf::RenderWindow window;

	AssetStore assets;
	GameState gameState;
	World world;
	std::optional<HUD> hud;

	ActionMap<Config::PlayerAction> actions;
	InputHandler<Config::PlayerAction> input;

	std::vector<sf::Text> startScreenTexts;
	std::vector<sf::Text> gameOverTexts;
	std::vector<sf::Text> levelCompleteTexts;
	std::vector<sf::Text> winTexts;
	std::optional<sf::Text> exitHintText;

	LevelData currentLevel;
};
