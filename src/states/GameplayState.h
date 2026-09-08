#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "core/world/World.h"
#include "entities/Pickup.h"
#include "gameplay/GameplayData.h"
#include "gameplay/BossEncounter.h"
#include "gameplay/GameplaySession.h"
#include "gameplay/TutorialDirector.h"
#include "gameplay/WaveDirector.h"
#include "rendering/GameplayBackground.h"
#include "rendering/GameplayEffects.h"
#include "rendering/GameplayPostProcessor.h"
#include "states/State.h"
#include "input/ActionMap.h"
#include "input/InputHandler.h"
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
	// A pending level -> level (or level -> menu) transition. Set when a fade
	// begins; fired by AdvancePendingTransition() once the fade finishes.
	enum class GameplayTransition
	{
		None,
		RestartLevel,
		NextLevel,
		RestartGame,
		TutorialComplete,
		ShipUpgrades,
		LevelSelect,
		MainMenu,
		CampaignComplete
	};

	// Which of the three loops this instance is running. Campaign is the
	// default (numbered levels, wave director, upgrades, saves); Horde and Run
	// are the endless variants chosen from the campaign menu. Set once in the
	// constructor and never changed. Sub-states of Campaign -- the tutorial,
	// level-select replays -- keep their own flags below.
	enum class GameMode
	{
		Campaign,
		Horde,
		Run
	};

	void SetupInput();

	// --- The steps of Update(), in call order -------------------------------
	// The bool-returning ones report "this step owned the frame -- stop here".
	void UpdatePresentation(float deltaTime);
	[[nodiscard]] bool ProcessPendingRuntimeCommand();
	[[nodiscard]] bool AdvancePendingTransition();
	void UpdateBackdrop(float deltaTime);
	[[nodiscard]] bool UpdateIntroSequences(float deltaTime);
	void UpdateActiveFrame(float deltaTime);
	void UpdateWaveAndModeProgression(float deltaTime);

	// --- Rendering / pause / transitions -----------------------------------
	void DrawScene(sf::RenderTarget& target);
	void OpenPauseMenu();
	void ResumeGameplaySounds();
	void BeginGameOver();
	void BeginGameOverTransition(UI::GameOverScreen::Action action);
	void BeginResultTransition(UI::ResultScreen::Action action);
	void RestartCurrentLevel();
	void Reset();

	// --- Spawning --------------------------------------------------------
	void SpawnPlayerIfNeeded();
	void SpawnConfiguredEnemy(
		const GameplayData::SpawnGroup& spawn,
		std::size_t spawnIndex,
		bool needToMaterialize = false,
		bool isBossReinforcement = false,
		std::optional<sf::Vector2f> forcedPosition = std::nullopt);
	void SpawnBossReinforcement(
		GameplayData::EnemyKind kind,
		std::optional<sf::Vector2f> position,
		std::optional<GameplayData::PickupKind> guaranteedPickup);
	void SpawnPickup(Pickup::Kind kind, sf::Vector2f position);
	[[nodiscard]] sf::Vector2f GetSafeSpawnPosition();
	[[nodiscard]] sf::Vector2f GetSafeEdgeSpawnPosition();

	// --- Tutorial --------------------------------------------------------
	void StartTutorial();
	void UpdateTutorial(float deltaTime);
	void ExecuteTutorialAction(TutorialDirector::Action action);
	void FinishTutorial();
	[[nodiscard]] TutorialDirector::Snapshot GetTutorialSnapshot() const;

	// --- Level completion / audio ----------------------------------------
	void BeginLevelCompleteAudio();
	void UpdateLevelCompleteAudio(float deltaTime);
	void CompleteCurrentLevel();
	void NextLevel();
	[[nodiscard]] UI::ResultScreen::Statistics FinalizeLevelStatistics();

	// --- Campaign progress / rewards / achievements ---------------------
	void RestoreCampaignProgress();
	void SaveCompletedLevel();
	void EvaluateEntryAchievements();
	void UnlockCompletionAchievements(int completedLevel);
	void SpawnLevel();
	void PrepareCampaignRewards(const GameplayData::LevelConfig& level);
	[[nodiscard]] std::vector<GameplayData::PickupKind>
		BuildCampaignBonusSequence(int levelNumber) const;

	// --- Horde mode -----------------------------------------------------
	void StartHorde();
	void StartNextHordeWave(bool needToMaterializeInitialSpawns, bool needToStartWaveIntro = true);
	[[nodiscard]] GameplayData::WaveConfig BuildHordeWave();
	[[nodiscard]] GameplayData::PickupKind TakeNextHordeBonus();
	void GrantHordeWaveUpgrade();

	// --- Run mode ------------------------------------------------------
	void StartRun();
	void UpdateRun(float deltaTime);
	void SpawnRunAsteroid();
	void SpawnRunEnemy(GameplayData::EnemyKind kind);

	// --- Waves / player animations ------------------------------------
	[[nodiscard]] const GameplayData::LevelConfig& GetPresentationLevel() const;
	void StartNextWave(bool needToMaterializeInitialSpawns, bool needToStartWaveIntro = true);
	void FinishWaveIntro();
	void UpdatePlayerSpawnAnimation(float deltaTime);
	void BeginPlayerWaveTeleport();
	void UpdatePlayerWaveTeleport(float deltaTime);
	void UpdateWaveMaterialization(float deltaTime);
	void UpdateTimeSlowdownPresentation(float deltaTime);
	[[nodiscard]] float GetWorldTimeScale() const noexcept;

	static constexpr float SpawnSafeRadius = 250.f;

	// --- Core systems ---------------------------------------------------
	const GameplayData& gameplayData;
	GameplaySession session;
	WaveDirector waveDirector;
	std::optional<BossEncounter> bossEncounter;
	ActionMap<Config::PlayerAction> actions;
	InputHandler<Config::PlayerAction> input;
	World world;

	// --- Rendering / screens ------------------------------------------
	Rendering::GameplayBackground background;
	Rendering::GameplayEffects effects;
	Rendering::GameplayPostProcessor postProcessor;
	UI::GlowingCursor crosshair;
	UI::GameOverScreen gameOverScreen;
	UI::ResultScreen resultScreen;
	UI::ScreenFade screenFade;
	UI::LevelIntro levelIntro;
	UI::WaveIntro waveIntro;
	std::optional<UI::HUD> hud;
	std::optional<TutorialDirector> tutorial;

	// --- Overall state ------------------------------------------------
	GameMode mode = GameMode::Campaign;
	GameplayTransition gameplayTransition = GameplayTransition::None;
	bool isTutorialActive = false;
	bool areGameplaySoundsPaused = false;
	bool hasBossVictorySequenceStarted = false;
	bool needToPreserveGameplayMusicOnDestruction = false;
	float levelGameplayElapsed = 0.f;
	float levelCompleteSoundRemaining = 0.f;
	float timeSlowdownVisualStrength = 0.f;

	// --- Player spawn-in / wave-clear teleport animations ------------
	bool isPlayerSpawnAnimating = false;
	bool isPlayerTeleportAnimating = false;
	bool hasPlayerTeleportMoved = false;
	bool isWaveClearDelayActive = false;
	float playerSpawnElapsed = 0.f;
	float playerTeleportElapsed = 0.f;
	float waveMaterializationElapsed = 0.f;
	float waveClearDelayRemaining = 0.f;
	std::vector<Entity*> materializingEnemies;

	// --- Campaign sub-state -----------------------------------------
	bool isMainCampaignRun = false;
	bool isSelectedLevelRun = false;
	bool isSelectedLevelAdvancingCampaign = false;
	std::unordered_map<std::size_t, int> campaignBonusDrops;
	std::unordered_map<std::size_t, std::string> campaignPartDrops;
	std::size_t campaignEnemySpawnOrdinal = 0u;
	int stationPathOffset = 0;
	int turretPathOffset = 0;

	// --- Horde sub-state -------------------------------------------
	bool isHordeHelperAvailable = true;
	int hordeCurrentWave = 1;
	int hordeWavesSurvived = 0;
	GameplayData::LevelConfig hordeLevel;
	std::vector<GameplayData::PickupKind> hordeBonusBag;

	// --- Run sub-state --------------------------------------------
	bool wasRunLaserTurretSpawned = false;
	bool wasRunReflectorSpawned = false;
	float runElapsed = 0.f;
	float runAsteroidTimer = 10.f;
	float runEnemyTimer = 15.f;
	int runAsteroidsSpawned = 0;
	int runShootersSpawned = 0;
};
