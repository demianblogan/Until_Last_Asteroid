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

	void SetupInput();
	void DrawScene(sf::RenderTarget& target);
	void OpenPauseMenu();
	void ResumeGameplaySounds();
	void BeginGameOver();
	void BeginGameOverTransition(UI::GameOverScreen::Action action);
	void BeginResultTransition(UI::ResultScreen::Action action);
	void RestartCurrentLevel();
	void SpawnPlayerIfNeeded();
	void SpawnConfiguredEnemy(
		const GameplayData::SpawnGroup& spawn,
		std::size_t spawnIndex,
		bool materialize = false,
		bool bossReinforcement = false,
		std::optional<sf::Vector2f> forcedPosition = std::nullopt);
	void SpawnBossReinforcement(
		GameplayData::EnemyKind kind,
		std::optional<sf::Vector2f> position,
		std::optional<GameplayData::PickupKind> guaranteedPickup);
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
	void EvaluateEntryAchievements();
	void UnlockCompletionAchievements(int completedLevel);
	[[nodiscard]] UI::ResultScreen::Statistics FinalizeLevelStatistics();
	void CompleteCurrentLevel();
	void NextLevel();
	void SpawnLevel();
	void PrepareCampaignRewards(const GameplayData::LevelConfig& level);
	[[nodiscard]] std::vector<GameplayData::PickupKind>
		BuildCampaignBonusSequence(int levelNumber) const;
	void StartHorde();
	void StartNextHordeWave(
		bool materializeInitialSpawns,
		bool startWaveIntro = true);
	[[nodiscard]] GameplayData::WaveConfig BuildHordeWave();
	[[nodiscard]] GameplayData::PickupKind TakeNextHordeBonus();
	void GrantHordeWaveUpgrade();
	void StartRun();
	void UpdateRun(float deltaTime);
	void SpawnRunAsteroid();
	void SpawnRunEnemy(GameplayData::EnemyKind kind);
	[[nodiscard]] const GameplayData::LevelConfig& GetPresentationLevel() const;
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
	std::optional<BossEncounter> bossEncounter;
	ActionMap<Config::PlayerAction> actions;
	InputHandler<Config::PlayerAction> input;
	Rendering::GameplayBackground background;
	Rendering::GameplayEffects effects;
	Rendering::GameplayPostProcessor postProcessor;
	World world;
	UI::GlowingCursor crosshair;
	UI::GameOverScreen gameOverScreen;
	UI::ResultScreen resultScreen;
	UI::ScreenFade screenFade;
	UI::LevelIntro levelIntro;
	UI::WaveIntro waveIntro;
	std::optional<UI::HUD> hud;
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
	bool hordeMode{ false };
	bool runMode{ false };
	bool hordeHelperAvailable{ true };
	int hordeCurrentWave{ 1 };
	int hordeWavesSurvived{ 0 };
	GameplayData::LevelConfig hordeLevel;
	std::vector<GameplayData::PickupKind> hordeBonusBag;
	std::unordered_map<std::size_t, int> campaignBonusDrops;
	std::unordered_map<std::size_t, std::string> campaignPartDrops;
	std::size_t campaignEnemySpawnOrdinal{ 0u };
	int stationPathOffset{ 0 };
	int turretPathOffset{ 0 };
	float runElapsed{ 0.f };
	float runAsteroidTimer{ 10.f };
	float runEnemyTimer{ 15.f };
	int runAsteroidsSpawned{ 0 };
	int runShootersSpawned{ 0 };
	bool runLaserTurretSpawned{ false };
	bool runReflectorSpawned{ false };
	bool bossVictorySequenceStarted{ false };
	bool preserveGameplayMusicOnDestruction{ false };
	bool mainCampaignRun{ false };
	std::vector<Entity*> materializingEnemies;
};
