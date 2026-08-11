#include "GameplayState.h"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "campaign/CampaignSaveManager.h"
#include "entities/Meteor.h"
#include "entities/Saucer.h"
#include "game/GameplayLaunch.h"
#include "settings/SettingsManager.h"
#include "systems/GamepadManager.h"
#include "utils/Random.h"

namespace
{
	constexpr sf::Color CrosshairGlowColor{ 25, 220, 255 };
	constexpr float GameplayFadeInDuration{ 0.45f };
	constexpr float GameplayFadeOutDuration{ 0.38f };
	constexpr float PlayerSpawnDuration{ 0.55f };
	constexpr float WaveMaterializationDuration{ 0.5f };
}

GameplayState::GameplayState(StateStack& stateStack, StateContext context)
	: State(stateStack, context)
	, gameplayData(context.assets.GetGameplayData())
	, input(actions)
	, background(context.assets, context.logicalSize)
	, effects(gameplayData.GetEffects(), context.assets)
	, postProcessor(context.assets, context.logicalSize)
	, world(static_cast<unsigned int>(context.logicalSize.x),
		static_cast<unsigned int>(context.logicalSize.y),
		context.assets, context.audio, session, context.gamepad)
	, crosshair(context.assets, Config::Texture::GameplayCrosshair,
		{ 32.f, 32.f }, CrosshairGlowColor)
	, gameOverScreen(context.assets, context.audio, context.gamepad, context.logicalSize)
	, resultScreen(context.assets, context.audio, context.gamepad, context.logicalSize)
	, screenFade(context.logicalSize)
	, waveIntro(context.assets, context.audio)
{
	world.SetWindow(context.window);
	context.window.setMouseCursorVisible(false);
	session.ConfigurePlayerHealth(gameplayData.GetPlayer().maximumHealth);
	session.ConfigureShield(
		gameplayData.GetPickups().shieldCapacity,
		gameplayData.GetPickups().shieldDuration);
	hud.emplace(context.assets, session);
	SetupInput();
	Reset();
	context.gameplayLaunch.tutorialRunning = false;
	const GameplayLaunchMode launchMode{ context.gameplayLaunch.mode };
	context.gameplayLaunch.mode = GameplayLaunchMode::ContinueCampaign;
	const CampaignProgress* progress{ context.campaignSave.GetProgress() };
	const bool resumeUnfinishedTutorial{
		launchMode == GameplayLaunchMode::ContinueCampaign &&
		progress != nullptr && !progress->tutorialCompleted };
	if (launchMode == GameplayLaunchMode::Tutorial || resumeUnfinishedTutorial)
		StartTutorial();
	else
	{
		RestoreCampaignProgress();
		SpawnLevel();
	}
	screenFade.StartFadeIn(GameplayFadeInDuration);
	context.audio.PlayMusic(Config::Music::GameplayBackground1);
}

GameplayState::~GameplayState()
{
	GetContext().gameplayLaunch.tutorialRunning = false;
	GetContext().audio.StopMusic(Config::Music::GameplayBackground1);
}

void GameplayState::SetupInput()
{
	using enum Config::PlayerAction;
	using enum InputAction::TriggerType;
	const ControlSettings& controls{ GetContext().settings.Get().controls };
	const auto addBinding{ [this](Config::PlayerAction action, const ControlBinding& binding)
	{
		if (binding.device == InputDevice::Keyboard)
			actions.AddBinding(action, InputAction(
				static_cast<sf::Keyboard::Key>(binding.code), WhileHeld));
		else
			actions.AddBinding(action, InputAction(
				static_cast<sf::Mouse::Button>(binding.code), WhileHeld));
	} };

	addBinding(Up, controls.moveUp);
	addBinding(Down, controls.moveDown);
	addBinding(Left, controls.moveLeft);
	addBinding(Right, controls.moveRight);
	addBinding(Fire, controls.fire);
}

void GameplayState::HandleEvent(const sf::Event& event)
{
	if (event.is<sf::Event::FocusLost>() && session.IsPlaying())
	{
		OpenPauseMenu();
		return;
	}

	if (screenFade.IsActive())
		return;

	if (gameOverScreen.IsActive())
	{
		if (const auto action{ gameOverScreen.HandleEvent(event, GetContext().window) })
			BeginGameOverTransition(*action);
		return;
	}
	if (resultScreen.IsActive())
	{
		if (const auto action{ resultScreen.HandleEvent(event, GetContext().window) })
			BeginResultTransition(*action);
		return;
	}

	if (session.IsPlaying() && GetContext().gamepad.IsPausePressed(event))
	{
		OpenPauseMenu();
		return;
	}

	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		if (key->code == sf::Keyboard::Key::Escape && session.IsPlaying())
		{
			OpenPauseMenu();
			return;
		}

	}

	if (waveIntro.IsActive() || playerSpawnAnimating)
		return;

	if (session.IsPlaying())
		world.HandlePlayerEvent(event);
}

void GameplayState::HandleRealtime()
{
	if (GetContext().gameplayLaunch.pendingCommand != GameplayRuntimeCommand::None)
		return;
	if (session.IsPlaying())
		ResumeGameplaySounds();
	if (screenFade.IsActive() || waveIntro.IsActive() || playerSpawnAnimating ||
		gameOverScreen.IsActive() || resultScreen.IsActive())
		return;
	if (session.IsPlaying())
		world.HandlePlayerRealtime();
}

void GameplayState::OpenPauseMenu()
{
	if (!gameplaySoundsPaused)
	{
		world.PauseActiveSounds();
		gameplaySoundsPaused = true;
	}
	RequestPush(StateId::Pause);
}

void GameplayState::ResumeGameplaySounds()
{
	if (!gameplaySoundsPaused)
		return;
	world.ResumePausedSounds();
	gameplaySoundsPaused = false;
}

void GameplayState::Update(float dt)
{
	screenFade.Update(dt);
	gameOverScreen.Update(dt);
	resultScreen.Update(dt);
	UpdateLevelCompleteAudio(dt);

	const GameplayRuntimeCommand runtimeCommand{
		GetContext().gameplayLaunch.pendingCommand };
	if (runtimeCommand != GameplayRuntimeCommand::None &&
		gameplayTransition == GameplayTransition::None)
	{
		GetContext().gameplayLaunch.pendingCommand = GameplayRuntimeCommand::None;
		world.StopActiveSounds();
		gameplaySoundsPaused = false;
		if (runtimeCommand == GameplayRuntimeCommand::SkipTutorial && tutorialActive)
			FinishTutorial();
		else if (runtimeCommand == GameplayRuntimeCommand::RestartLevel)
		{
			gameplayTransition = GameplayTransition::RestartLevel;
			screenFade.StartFadeOut(GameplayFadeOutDuration);
		}
		return;
	}

	if (gameplayTransition != GameplayTransition::None)
	{
		if (!screenFade.IsActive())
		{
			const GameplayTransition completedTransition{ gameplayTransition };
			gameplayTransition = GameplayTransition::None;
			if (completedTransition == GameplayTransition::RestartLevel)
			{
				RestartCurrentLevel();
				screenFade.StartFadeIn(GameplayFadeInDuration);
			}
			else if (completedTransition == GameplayTransition::NextLevel)
			{
				NextLevel();
				screenFade.StartFadeIn(GameplayFadeInDuration);
			}
			else if (completedTransition == GameplayTransition::RestartGame)
			{
				static_cast<void>(GetContext().campaignSave.StartNewCampaign());
				Reset();
				SpawnLevel();
				screenFade.StartFadeIn(GameplayFadeInDuration);
			}
			else if (completedTransition == GameplayTransition::TutorialComplete)
			{
				world.Clear();
				effects.Clear();
				tutorial.reset();
				tutorialActive = false;
				session.StartAtLevel(1, 0);
				SpawnLevel();
				screenFade.StartFadeIn(GameplayFadeInDuration);
			}
			else if (completedTransition == GameplayTransition::MainMenu)
			{
				GetContext().audio.StopMusic(Config::Music::GameplayBackground1);
				RequestClear();
				RequestPush(StateId::MainMenu);
			}
		}
		return;
	}

	if (!gameOverScreen.IsActive() && !resultScreen.IsActive())
	{
		background.Update(dt);
		crosshair.Update(dt);
	}
	if (screenFade.IsActive())
		return;
	if (waveIntro.IsActive())
	{
		UpdateWaveMaterialization(dt);
		if (waveIntro.Update(dt))
			FinishWaveIntro();
		return;
	}
	if (playerSpawnAnimating)
	{
		UpdatePlayerSpawnAnimation(dt);
		return;
	}
	if (!session.IsPlaying())
	{
		effects.Update(dt, world,
			GetContext().settings.Get().gameplay.screenShake,
			GetContext().settings.Get().gameplay.showScorePopups);
		return;
	}

	session.Update(dt);
	world.Update(dt);
	effects.Update(dt, world,
		GetContext().settings.Get().gameplay.screenShake,
		GetContext().settings.Get().gameplay.showScorePopups);
	if (hud)
		hud->Update(dt);
	if (session.IsGameOver() && !gameOverScreen.IsActive())
		BeginGameOver();
	if (!session.IsPlaying())
		return;
	if (tutorialActive)
	{
		UpdateTutorial(dt);
		return;
	}

	waveDirector.Update(dt, [this](GameplayData::EnemyKind kind)
	{
		SpawnConfiguredEnemy(kind);
	});

	if (waveDirector.IsDeploymentComplete() && world.IsCleared())
	{
		if (waveDirector.HasMoreWaves())
		{
			StartNextWave(true);
			return;
		}

		SaveCompletedLevel();
		BeginLevelCompleteAudio();
		if (session.GetLevel() >= gameplayData.GetLevelCount())
		{
			session.SetWin();
			resultScreen.Start(ResultScreen::Mode::Victory,
				session.GetLevel(), session.GetScore());
		}
		else
		{
			session.SetLevelComplete();
			resultScreen.Start(ResultScreen::Mode::LevelComplete,
				session.GetLevel(), session.GetScore());
		}
	}
}

void GameplayState::Render()
{
	auto& window{ GetContext().window };
	if (GetContext().settings.Get().graphics.postEffects)
	{
		postProcessor.Render(
			window,
			gameplayData.GetLevel(session.GetLevel()).postProcess,
			effects.GetPostProcessState(),
			[this](sf::RenderTarget& target) { DrawScene(target); });
	}
	else
	{
		DrawScene(window);
	}

	waveIntro.Draw(window);

	if (session.IsPlaying())
	{
		if (hud) hud->Draw(window);
		if (tutorialActive && tutorial)
			tutorial->Draw(window);
	}
	else if (session.IsGameOver())
		gameOverScreen.Draw(window);
	else if (resultScreen.IsActive())
		resultScreen.Draw(window);
}

void GameplayState::DrawScene(sf::RenderTarget& target)
{
	target.draw(background);

	sf::RenderStates worldStates;
	worldStates.transform.translate(effects.GetCameraOffset());
	effects.DrawBehindEntities(target, worldStates);
	target.draw(world, worldStates);
	effects.DrawAboveEntities(target, worldStates);
}

void GameplayState::RenderOverlay()
{
	if (gameOverScreen.IsActive())
		gameOverScreen.DrawCursor(GetContext().window);
	else if (resultScreen.IsActive())
		resultScreen.DrawCursor(GetContext().window);
	else if (!waveIntro.IsActive() && !playerSpawnAnimating &&
		!world.GetPlayerGamepadAimPoint())
		crosshair.Draw(GetContext().window);
	screenFade.Draw(GetContext().window);
}

void GameplayState::SpawnPlayerIfNeeded()
{
	if (!world.HasPlayer() && !session.IsGameOver())
		world.SpawnPlayer(GetContext().assets, input);
}

void GameplayState::SpawnConfiguredEnemy(GameplayData::EnemyKind kind, bool materialize)
{
	using Kind = GameplayData::EnemyKind;
	std::unique_ptr<Entity> entity;
	bool edgeSpawn{ false };

	switch (kind)
	{
	case Kind::BigMeteor:
		entity = std::make_unique<Meteor>(GetContext().assets, world, Meteor::Size::Big);
		break;
	case Kind::SmallMeteor:
		entity = std::make_unique<Meteor>(GetContext().assets, world, Meteor::Size::Small);
		break;
	case Kind::Kamikaze:
		entity = std::make_unique<Saucer>(GetContext().assets, world, Saucer::Mode::Kamikaze);
		edgeSpawn = true;
		break;
	case Kind::Shooter:
		entity = std::make_unique<Saucer>(GetContext().assets, world, Saucer::Mode::Shooter);
		edgeSpawn = true;
		break;
	default:
		std::unreachable();
	}

	entity->SetPosition(edgeSpawn ? GetSafeEdgeSpawnPosition() : GetSafeSpawnPosition());
	if (materialize)
	{
		entity->SetPresentation(0.7f, 0.f, sf::Color(80, 225, 255));
		materializingEnemies.push_back(entity.get());
	}
	world.Spawn(std::move(entity));
}

void GameplayState::SpawnPickup(Pickup::Kind kind, sf::Vector2f position)
{
	auto pickup{ std::make_unique<Pickup>(GetContext().assets, world, kind) };
	pickup->SetPosition(position);
	world.Spawn(std::move(pickup));
}

void GameplayState::StartTutorial()
{
	world.Clear();
	effects.Clear();
	session.Reset();
	gameOverScreen.Reset();
	resultScreen.Reset();
	gameplayTransition = GameplayTransition::None;
	tutorialActive = true;
	GetContext().gameplayLaunch.tutorialRunning = true;

	const auto& level{ gameplayData.GetLevel(1) };
	background.SetTheme(level.background, level.backgroundBrightness);
	SpawnPlayerIfNeeded();
	world.CommitPendingEntities();
	world.SetPlayerSpawnPresentation(1.f);

	tutorial.emplace(
		GetContext().assets,
		GetContext().logicalSize,
		GetContext().settings.Get().controls);
	tutorial->Start(GetTutorialSnapshot());
	if (hud)
		hud->Update(0.f);
}

void GameplayState::UpdateTutorial(float deltaTime)
{
	if (!tutorial)
		return;
	if (const auto action{ tutorial->Update(deltaTime, GetTutorialSnapshot()) })
		ExecuteTutorialAction(*action);
}

void GameplayState::ExecuteTutorialAction(TutorialDirector::Action action)
{
	using enum TutorialDirector::Action;
	switch (action)
	{
	case SpawnBigMeteor:
	{
		auto meteor{ std::make_unique<Meteor>(
			GetContext().assets, world, Meteor::Size::Big) };
		meteor->SetPosition({ world.GetWidth() - 220.f, world.GetHeight() * 0.36f });
		meteor->SetVelocity({ -85.f, 25.f });
		world.Spawn(std::move(meteor));
		world.CommitPendingEntities();
		break;
	}
	case HighlightScore:
		if (hud) hud->HighlightScore(5.f);
		break;
	case HighlightArmor:
		if (hud) hud->HighlightHealth(5.f);
		break;
	case HighlightShield:
		if (hud) hud->HighlightShield(5.f);
		break;
	case SpawnShooter:
	{
		auto shooter{ std::make_unique<Saucer>(
			GetContext().assets, world, Saucer::Mode::Shooter) };
		shooter->SetPosition({ 180.f, world.GetHeight() * 0.42f });
		shooter->SetVelocity({ 115.f, 45.f });
		world.Spawn(std::move(shooter));
		world.CommitPendingEntities();
		break;
	}
	case SpawnShield:
	{
		const sf::Vector2f playerPosition{ world.GetPlayerPosition() };
		sf::Vector2f pickupPosition{ world.GetWidth() * 0.72f, world.GetHeight() * 0.68f };
		const sf::Vector2f delta{ pickupPosition - playerPosition };
		if (delta.x * delta.x + delta.y * delta.y < 90000.f)
			pickupPosition = { world.GetWidth() * 0.25f, world.GetHeight() * 0.72f };
		SpawnPickup(Pickup::Kind::Shield, pickupPosition);
		world.CommitPendingEntities();
		break;
	}
	case Complete:
		FinishTutorial();
		break;
	}
}

void GameplayState::FinishTutorial()
{
	GetContext().gameplayLaunch.tutorialRunning = false;
	if (CampaignProgress* progress{ GetContext().campaignSave.EditProgress() })
	{
		progress->tutorialCompleted = true;
		static_cast<void>(GetContext().campaignSave.Save());
	}
	gameplayTransition = GameplayTransition::TutorialComplete;
	screenFade.StartFadeOut(GameplayFadeOutDuration);
}

void GameplayState::BeginLevelCompleteAudio()
{
	GetContext().audio.PlaySound(
		Config::Sound::LevelComplete,
		SoundGroup::Gameplay,
		100.f,
		1.f,
		SoundPlayback::Restart);
	GetContext().audio.PlayMusic(Config::Music::GameplayBackground1, true, 50.f);
	levelCompleteSoundRemaining = GetContext().assets.Sounds()
		.Get(Config::Sound::LevelComplete).getDuration().asSeconds();
}

void GameplayState::UpdateLevelCompleteAudio(float deltaTime)
{
	if (levelCompleteSoundRemaining <= 0.f)
		return;

	levelCompleteSoundRemaining = std::max(
		0.f, levelCompleteSoundRemaining - deltaTime);
	if (levelCompleteSoundRemaining <= 0.f)
		GetContext().audio.PlayMusic(Config::Music::GameplayBackground1, true, 100.f);
}

TutorialDirector::Snapshot GameplayState::GetTutorialSnapshot() const
{
	const World::Statistics& statistics{ world.GetStatistics() };
	return {
		world.GetPlayerPosition(),
		statistics.playerShotsFired,
		statistics.bigMeteorsDestroyed,
		statistics.smallMeteorsDestroyed,
		statistics.shootersDestroyed,
		statistics.shieldPickupsCollected };
}

void GameplayState::Reset()
{
	world.Clear();
	effects.Clear();
	session.Reset();
	gameOverScreen.Reset();
	resultScreen.Reset();
	gameplayTransition = GameplayTransition::None;
	playerSpawnElapsed = 0.f;
	waveMaterializationElapsed = 0.f;
	playerSpawnAnimating = false;
	materializingEnemies.clear();
	tutorial.reset();
	tutorialActive = false;
	GetContext().gameplayLaunch.tutorialRunning = false;
	if (hud) hud->Update(0.f);
}

void GameplayState::RestoreCampaignProgress()
{
	const CampaignProgress* progress{ GetContext().campaignSave.GetProgress() };
	if (progress == nullptr)
		return;

	const int level{ std::clamp(progress->currentLevel, 1, gameplayData.GetLevelCount()) };
	const int score{ std::max(0, progress->campaignScore) };
	session.StartAtLevel(level, score);
}

void GameplayState::SaveCompletedLevel()
{
	CampaignProgress* progress{ GetContext().campaignSave.EditProgress() };
	if (progress == nullptr)
		return;

	const int completedLevel{ session.GetLevel() };
	if (std::ranges::find(progress->completedLevels, completedLevel) ==
		progress->completedLevels.end())
	{
		progress->completedLevels.push_back(completedLevel);
	}

	int& bestScore{ progress->levelBestScores[completedLevel] };
	bestScore = std::max(bestScore, session.GetLevelScore());
	progress->campaignScore = session.GetScore();
	progress->highestUnlockedLevel = std::max(
		progress->highestUnlockedLevel,
		std::min(completedLevel + 1, gameplayData.GetLevelCount()));

	if (completedLevel >= gameplayData.GetLevelCount())
	{
		progress->campaignCompleted = true;
		progress->currentLevel = gameplayData.GetLevelCount();
	}
	else
	{
		progress->currentLevel = completedLevel + 1;
	}

	static_cast<void>(GetContext().campaignSave.Save());
}

void GameplayState::BeginGameOver()
{
	world.StopActiveSounds();
	world.AddSound(Config::Sound::ShipExplosion);
	GetContext().audio.PauseMusic(Config::Music::GameplayBackground1);
	gameOverScreen.Start(session.GetScore());
}

void GameplayState::BeginGameOverTransition(GameOverScreen::Action action)
{
	gameplayTransition = action == GameOverScreen::Action::RestartLevel
		? GameplayTransition::RestartLevel
		: GameplayTransition::MainMenu;
	screenFade.StartFadeOut(GameplayFadeOutDuration);
}

void GameplayState::BeginResultTransition(ResultScreen::Action action)
{
	if (action == ResultScreen::Action::MainMenu)
		gameplayTransition = GameplayTransition::MainMenu;
	else
		gameplayTransition = resultScreen.GetMode() == ResultScreen::Mode::Victory
			? GameplayTransition::RestartGame
			: GameplayTransition::NextLevel;
	screenFade.StartFadeOut(GameplayFadeOutDuration);
}

void GameplayState::RestartCurrentLevel()
{
	if (tutorialActive)
	{
		StartTutorial();
		GetContext().audio.ResumeMusic(Config::Music::GameplayBackground1);
		return;
	}
	world.Clear();
	effects.Clear();
	session.RestartLevel();
	gameOverScreen.Reset();
	resultScreen.Reset();
	SpawnLevel();
	if (hud)
		hud->Update(0.f);
	GetContext().audio.ResumeMusic(Config::Music::GameplayBackground1);
}

void GameplayState::NextLevel()
{
	if (session.GetLevel() >= gameplayData.GetLevelCount())
	{
		session.SetWin();
		return;
	}
	world.Clear();
	effects.Clear();
	session.NextLevel();
	resultScreen.Reset();
	SpawnLevel();
}

void GameplayState::SpawnLevel()
{
	const auto& level{ gameplayData.GetLevel(session.GetLevel()) };
	background.SetTheme(level.background, level.backgroundBrightness);
	waveDirector.LoadLevel(level);
	StartNextWave(false);
}

void GameplayState::StartNextWave(bool materializeInitialSpawns)
{
	materializingEnemies.clear();
	waveMaterializationElapsed = 0.f;
	static_cast<void>(waveDirector.StartNextWave([this, materializeInitialSpawns](
		GameplayData::EnemyKind kind)
	{
		SpawnConfiguredEnemy(kind, materializeInitialSpawns);
	}));
	world.CommitPendingEntities();
	waveIntro.Start(waveDirector.GetCurrentWaveNumber());
}

void GameplayState::FinishWaveIntro()
{
	if (world.HasPlayer())
		return;

	SpawnPlayerIfNeeded();
	world.CommitPendingEntities();
	playerSpawnElapsed = 0.f;
	playerSpawnAnimating = true;
	world.SetPlayerSpawnPresentation(0.f);
}

void GameplayState::UpdatePlayerSpawnAnimation(float deltaTime)
{
	playerSpawnElapsed = std::min(PlayerSpawnDuration, playerSpawnElapsed + deltaTime);
	const float progress{ playerSpawnElapsed / PlayerSpawnDuration };
	world.SetPlayerSpawnPresentation(progress);
	if (playerSpawnElapsed >= PlayerSpawnDuration)
		playerSpawnAnimating = false;
}

void GameplayState::UpdateWaveMaterialization(float deltaTime)
{
	if (materializingEnemies.empty())
		return;

	waveMaterializationElapsed = std::min(
		WaveMaterializationDuration,
		waveMaterializationElapsed + deltaTime);
	const float progress{ waveMaterializationElapsed / WaveMaterializationDuration };
	const float eased{ progress * progress * (3.f - 2.f * progress) };
	const auto channel{ static_cast<std::uint8_t>(80.f + 175.f * eased) };
	const sf::Color tint{ channel, static_cast<std::uint8_t>(225.f + 30.f * eased), 255u };
	for (Entity* enemy : materializingEnemies)
		enemy->SetPresentation(0.7f + 0.3f * eased, eased, tint);

	if (waveMaterializationElapsed >= WaveMaterializationDuration)
		materializingEnemies.clear();
}

sf::Vector2f GameplayState::GetSafeSpawnPosition()
{
	constexpr int MaximumAttempts{ 50 };
	for (int i{ 0 }; i < MaximumAttempts; ++i)
	{
		const sf::Vector2f position{ Random::Float(0.f, static_cast<float>(world.GetWidth())),
			Random::Float(0.f, static_cast<float>(world.GetHeight())) };
		const sf::Vector2f protectedPosition{ world.HasPlayer()
			? world.GetPlayerPosition()
			: sf::Vector2f{ world.GetWidth() * 0.5f, world.GetHeight() * 0.5f } };
		const sf::Vector2f delta{ position - protectedPosition };
		if (delta.x * delta.x + delta.y * delta.y > SpawnSafeRadius * SpawnSafeRadius)
			return position;
	}
	return { Random::Float(0.f, static_cast<float>(world.GetWidth())),
		Random::Float(0.f, static_cast<float>(world.GetHeight())) };
}

sf::Vector2f GameplayState::GetSafeEdgeSpawnPosition()
{
	const auto spawnAtEdge{ [this]()
	{
		const float width{ static_cast<float>(world.GetWidth()) };
		const float height{ static_cast<float>(world.GetHeight()) };
		switch (Random::Int(0, 3))
		{
		case 0: return sf::Vector2f{ 0.f, Random::Float(0.f, height) };
		case 1: return sf::Vector2f{ width, Random::Float(0.f, height) };
		case 2: return sf::Vector2f{ Random::Float(0.f, width), 0.f };
		case 3: return sf::Vector2f{ Random::Float(0.f, width), height };
		default: std::unreachable();
		}
	} };

	constexpr int MaximumAttempts{ 50 };
	for (int i{ 0 }; i < MaximumAttempts; ++i)
	{
		const sf::Vector2f position{ spawnAtEdge() };
		if (!world.HasPlayer()) return position;
		const sf::Vector2f delta{ position - world.GetPlayerPosition() };
		if (delta.x * delta.x + delta.y * delta.y > SpawnSafeRadius * SpawnSafeRadius)
			return position;
	}
	return spawnAtEdge();
}
