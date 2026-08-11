#pragma once

#include <optional>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "core/World.h"
#include "game/GameplayData.h"
#include "game/GameplaySession.h"
#include "rendering/GameplayBackground.h"
#include "rendering/GameplayEffects.h"
#include "rendering/GameplayPostProcessor.h"
#include "states/State.h"
#include "systems/ActionMap.h"
#include "systems/InputHandler.h"
#include "ui/GlowingCursor.h"
#include "ui/GameOverScreen.h"
#include "ui/HUD.h"
#include "ui/ResultScreen.h"
#include "ui/ScreenFade.h"
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
	void RenderOverlay() override;

private:
	enum class GameplayTransition
	{
		None,
		RestartLevel,
		NextLevel,
		RestartGame,
		MainMenu
	};

	struct RuntimeWave
	{
		GameplayData::WaveConfig config;
		float timer{ 0.f };
		int repetitionsSpawned{ 0 };
	};

	void SetupInput();
	void DrawScene(sf::RenderTarget& target);
	void OpenPauseMenu();
	void ResumeGameplaySounds();
	void BeginGameOver();
	void BeginGameOverTransition(GameOverScreen::Action action);
	void BeginResultTransition(ResultScreen::Action action);
	void RestartCurrentLevel();
	void SpawnPlayerIfNeeded();
	void SpawnConfiguredEnemy(GameplayData::EnemyKind kind);
	void Reset();
	void NextLevel();
	void SpawnLevel();
	[[nodiscard]] sf::Vector2f GetSafeSpawnPosition();
	[[nodiscard]] sf::Vector2f GetSafeEdgeSpawnPosition();

	static constexpr float SpawnSafeRadius{ 250.f };

	const GameplayData& gameplayData;
	GameplaySession session;
	ActionMap<Config::PlayerAction> actions;
	InputHandler<Config::PlayerAction> input;
	GameplayBackground background;
	GameplayEffects effects;
	GameplayPostProcessor postProcessor;
	World world;
	GlowingCursor crosshair;
	GameOverScreen gameOverScreen;
	ResultScreen resultScreen;
	ScreenFade screenFade;
	std::optional<HUD> hud;
	std::vector<RuntimeWave> currentWaves;
	bool gameplaySoundsPaused{ false };
	GameplayTransition gameplayTransition{ GameplayTransition::None };
};
