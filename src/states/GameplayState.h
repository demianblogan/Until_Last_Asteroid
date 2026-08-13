#pragma once

#include <optional>
#include <vector>
#include <SFML/System/Vector2.hpp>

#include "core/World.h"
#include "entities/Pickup.h"
#include "game/GameplayData.h"
#include "game/GameplaySession.h"
#include "game/TutorialDirector.h"
#include "game/WaveDirector.h"
#include "rendering/GameplayBackground.h"
#include "rendering/GameplayEffects.h"
#include "rendering/GameplayPostProcessor.h"
#include "states/State.h"
#include "systems/ActionMap.h"
#include "systems/InputHandler.h"
#include "ui/GlowingCursor.h"
#include "ui/GameOverScreen.h"
#include "ui/HUD.h"
#include "ui/LevelIntro.h"
#include "ui/ResultScreen.h"
#include "ui/ScreenFade.h"
#include "ui/WaveIntro.h"
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
		TutorialComplete,
		ShipUpgrades,
		LevelSelect,
		MainMenu
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
	void SpawnConfiguredEnemy(
		const GameplayData::SpawnGroup& spawn,
		std::size_t spawnIndex,
		bool materialize = false);
	void SpawnPickup(Pickup::Kind kind, sf::Vector2f position);
	void StartTutorial();
	void UpdateTutorial(float deltaTime);
	void ExecuteTutorialAction(TutorialDirector::Action action);
	void FinishTutorial();
	void BeginLevelCompleteAudio();
	void UpdateLevelCompleteAudio(float deltaTime);
	[[nodiscard]] TutorialDirector::Snapshot GetTutorialSnapshot() const;
	void Reset();
	void RestoreCampaignProgress();
	void SaveCompletedLevel();
	[[nodiscard]] ResultScreen::Statistics FinalizeLevelStatistics();
	void CompleteCurrentLevel();
#ifdef _DEBUG
	void DebugCompleteCurrentLevel();
#endif
	void NextLevel();
	void SpawnLevel();
	void StartNextWave(bool materializeInitialSpawns, bool startWaveIntro = true);
	void FinishWaveIntro();
	void UpdatePlayerSpawnAnimation(float deltaTime);
	void BeginPlayerWaveTeleport();
	void UpdatePlayerWaveTeleport(float deltaTime);
	void UpdateWaveMaterialization(float deltaTime);
	void UpdateTimeSlowdownPresentation(float deltaTime);
	[[nodiscard]] float GetWorldTimeScale() const noexcept;
	[[nodiscard]] sf::Vector2f GetSafeSpawnPosition();
	[[nodiscard]] sf::Vector2f GetSafeEdgeSpawnPosition();

	static constexpr float SpawnSafeRadius{ 250.f };

	const GameplayData& gameplayData;
	GameplaySession session;
	WaveDirector waveDirector;
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
	LevelIntro levelIntro;
	WaveIntro waveIntro;
	std::optional<HUD> hud;
	std::optional<TutorialDirector> tutorial;
	bool tutorialActive{ false };
	bool gameplaySoundsPaused{ false };
	float levelCompleteSoundRemaining{ 0.f };
	GameplayTransition gameplayTransition{ GameplayTransition::None };
	float playerSpawnElapsed{ 0.f };
	float playerTeleportElapsed{ 0.f };
	float waveMaterializationElapsed{ 0.f };
	float waveClearDelayRemaining{ 0.f };
	float timeSlowdownVisualStrength{ 0.f };
	float levelGameplayElapsed{ 0.f };
	bool playerSpawnAnimating{ false };
	bool playerTeleportAnimating{ false };
	bool playerTeleportMoved{ false };
	bool waveClearDelayActive{ false };
	bool selectedLevelRun{ false };
	bool selectedLevelAdvancesCampaign{ false };
	std::vector<Entity*> materializingEnemies;
};
