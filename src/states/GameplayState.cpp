#include "GameplayState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string_view>
#include <utility>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "campaign/CampaignSaveManager.h"
#include "entities/LaserTurret.h"
#include "entities/Meteor.h"
#include "entities/MissileCarrier.h"
#include "entities/Part.h"
#include "entities/ReflectorGunship.h"
#include "entities/Saucer.h"
#include "entities/ShooterStation.h"
#include "entities/Spinner.h"
#include "game/GameplayLaunch.h"
#include "records/RecordsManager.h"
#include "settings/SettingsManager.h"
#include "systems/GamepadManager.h"
#include "utils/Random.h"

namespace
{
	constexpr sf::Color CrosshairGlowColor{ 25, 220, 255 };
	constexpr float GameplayFadeInDuration{ 0.45f };
	constexpr float GameplayFadeOutDuration{ 0.38f };
	constexpr float PlayerSpawnDuration{ 0.55f };
	constexpr float PlayerTeleportDuration{ 0.8f };
	constexpr float PlayerTeleportMoveTime{ 0.34f };
	constexpr float WaveMaterializationDuration{ 0.5f };
	constexpr float WaveClearDelayDuration{ 2.f };
	constexpr float TimeSlowdownFadeSpeed{ 4.f };
	constexpr float RunAsteroidInterval{ 10.f };
	constexpr float RunEnemyInterval{ 15.f };
	constexpr int RunAsteroidLimit{ 10 };
	constexpr int RunShooterLimit{ 3 };
	constexpr float ArmorBonusThreshold{ 0.75f };
	constexpr int ArmorBonusPoints{ 500 };
	constexpr int AccuracyBonusPoints{ 600 };
	constexpr int AllPartsBonusPoints{ 1000 };
	struct HordeBackground
	{
		std::string_view theme;
		float brightness;
	};
	constexpr std::array<HordeBackground, 9> HordeBackgrounds{
		HordeBackground{ "blue_nebula_region", 0.65f },
		HordeBackground{ "emerald_aurora_region", 0.64f },
		HordeBackground{ "violet_clouds_region", 0.67f },
		HordeBackground{ "rose_nursery_region", 0.62f },
		HordeBackground{ "asteroid_belt_region", 0.62f },
		HordeBackground{ "frozen_expanse_region", 0.66f },
		HordeBackground{ "red_storm_region", 0.60f },
		HordeBackground{ "ion_storm_region", 0.61f },
		HordeBackground{ "deep_void_region", 0.66f }
	};

	Config::Music GetGameplayMusic(int level) noexcept
	{
		if (level >= 7) return Config::Music::GameplayBackground3;
		if (level >= 4) return Config::Music::GameplayBackground2;
		return Config::Music::GameplayBackground1;
	}
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
	, levelIntro(context.assets, context.logicalSize)
	, waveIntro(context.assets)
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
	const bool campaignMode{
		launchMode != GameplayLaunchMode::Horde &&
		launchMode != GameplayLaunchMode::Run };
	if (campaignMode && progress != nullptr)
		session.ConfigureUpgrades(progress->upgrades);
	else
		session.ConfigureUpgrades({});
	session.ConfigurePlayerHealth(static_cast<int>(std::lround(
		static_cast<float>(gameplayData.GetPlayer().maximumHealth) *
		session.GetArmorMultiplier())));
	session.GetPlayerHealth().Reset();
	if (campaignMode && progress != nullptr)
		session.ConfigureParts(progress->partsBalance, progress->collectedPartIds);
	const bool resumeUnfinishedTutorial{
		launchMode == GameplayLaunchMode::ContinueCampaign &&
		progress != nullptr && !progress->tutorialCompleted };
	if (launchMode == GameplayLaunchMode::Horde)
	{
		hordeMode = true;
		StartHorde();
	}
	else if (launchMode == GameplayLaunchMode::Run)
	{
		runMode = true;
		StartRun();
	}
	else if (launchMode == GameplayLaunchMode::SelectedLevel)
	{
		selectedLevelRun = true;
		const int highestUnlocked{ progress != nullptr
			? std::max(1, progress->highestUnlockedLevel)
			: 1 };
		const int selectedLevel{ std::clamp(
			context.gameplayLaunch.selectedLevel,
			1,
			std::min(highestUnlocked, gameplayData.GetLevelCount())) };
		selectedLevelAdvancesCampaign = progress != nullptr &&
			selectedLevel == progress->currentLevel &&
			std::ranges::find(progress->completedLevels, selectedLevel) ==
				progress->completedLevels.end();
		session.StartAtLevel(selectedLevel);
		SpawnLevel();
	}
	else if (launchMode == GameplayLaunchMode::Tutorial || resumeUnfinishedTutorial)
		StartTutorial();
	else
	{
		RestoreCampaignProgress();
		SpawnLevel();
	}
	if (hud)
	{
		hud->SetPartsVisible(!hordeMode && !runMode);
		hud->Update(0.f);
	}
	screenFade.StartFadeIn(GameplayFadeInDuration);
	if (tutorialActive)
		context.audio.PlayGameplayMusic(GetGameplayMusic(session.GetLevel()));
}

GameplayState::~GameplayState()
{
	GetContext().gameplayLaunch.tutorialRunning = false;
	GetContext().audio.SetGameplayPitch(1.f);
	GetContext().audio.StopGameplayMusic();
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
#ifdef _DEBUG
		if (key->code == sf::Keyboard::Key::F1 && session.IsPlaying() &&
			!tutorialActive && !hordeMode && !runMode)
		{
			DebugCompleteCurrentLevel();
			return;
		}
		if (key->code == sf::Keyboard::Key::F2 && session.IsPlaying() &&
			!tutorialActive && !runMode && world.HasPlayer())
		{
			sf::Vector2f position{ world.GetPlayerPosition() + sf::Vector2f{ 180.f, 0.f } };
			if (position.x > static_cast<float>(world.GetWidth()) - 60.f)
				position.x = world.GetPlayerPosition().x - 180.f;
			if (world.SpawnHelperPickup(position))
				world.CommitPendingEntities();
			return;
		}
		if (key->code == sf::Keyboard::Key::F3 && session.IsPlaying() &&
			!tutorialActive && world.HasPlayer())
		{
			auto reflector{ std::make_unique<ReflectorGunship>(
				GetContext().assets, world) };
			reflector->SetPosition(GetSafeEdgeSpawnPosition());
			reflector->ConfigureApproachTarget({
				world.GetWidth() * 0.5f, world.GetHeight() * 0.5f });
			world.Spawn(std::move(reflector));
			world.CommitPendingEntities();
			return;
		}
#endif
		if (key->code == sf::Keyboard::Key::Escape && session.IsPlaying())
		{
			OpenPauseMenu();
			return;
		}

	}

	if (levelIntro.IsActive() || waveIntro.IsActive() ||
		playerSpawnAnimating || waveClearDelayActive)
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
	if (screenFade.IsActive() || levelIntro.IsActive() ||
		waveIntro.IsActive() || playerSpawnAnimating ||
		waveClearDelayActive ||
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
	UpdateTimeSlowdownPresentation(dt);

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
				session.StartAtLevel(1);
				SpawnLevel();
				screenFade.StartFadeIn(GameplayFadeInDuration);
			}
			else if (completedTransition == GameplayTransition::LevelSelect)
			{
				GetContext().audio.StopGameplayMusic();
				RequestClear();
				RequestPush(StateId::CampaignMenu);
				RequestPush(StateId::LevelSelect);
			}
			else if (completedTransition == GameplayTransition::ShipUpgrades)
			{
				GetContext().audio.StopGameplayMusic();
				RequestClear();
				RequestPush(StateId::ShipUpgrades);
			}
			else if (completedTransition == GameplayTransition::MainMenu)
			{
				GetContext().audio.StopGameplayMusic();
				RequestClear();
				RequestPush(StateId::MainMenu);
			}
		}
		return;
	}

	if (!gameOverScreen.IsActive() && !resultScreen.IsActive())
	{
		background.Update(dt * GetWorldTimeScale());
		crosshair.Update(dt);
	}
	if (screenFade.IsActive())
		return;
	if (levelIntro.IsActive())
	{
		if (levelIntro.Update(dt))
		{
			if (runMode)
				FinishWaveIntro();
			else if (hordeMode)
				waveIntro.Start(hordeCurrentWave);
			else
				waveIntro.Start(
					waveDirector.GetCurrentWaveNumber(),
					!waveDirector.HasMoreWaves());
		}
		return;
	}
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

	if (!waveClearDelayActive)
		levelGameplayElapsed += dt;
	if (runMode)
		UpdateRun(dt);
	if (!waveClearDelayActive)
		session.Update(dt);
	const float worldTimeScale{ GetWorldTimeScale() };
	world.Update(dt, worldTimeScale);
	effects.Update(dt * worldTimeScale, world,
		GetContext().settings.Get().gameplay.screenShake,
		GetContext().settings.Get().gameplay.showScorePopups);
	if (hud)
	{
		if (runMode)
			hud->SetSurvivalTime(runElapsed);
		hud->Update(dt);
	}
	if (session.IsGameOver() && !gameOverScreen.IsActive())
		BeginGameOver();
	if (!session.IsPlaying())
		return;
	if (tutorialActive)
	{
		UpdateTutorial(dt);
		return;
	}
	if (runMode)
		return;
	if (waveClearDelayActive)
	{
		UpdatePlayerWaveTeleport(dt);
		waveClearDelayRemaining = std::max(0.f, waveClearDelayRemaining - dt);
		if (waveClearDelayRemaining <= 0.f)
		{
			waveClearDelayActive = false;
			world.ClearProjectiles();
			if (hordeMode)
				StartNextHordeWave(true);
			else
				StartNextWave(true);
		}
		return;
	}

	waveDirector.Update(dt * worldTimeScale, [this](
		const GameplayData::SpawnGroup& spawn, std::size_t spawnIndex)
	{
		SpawnConfiguredEnemy(spawn, spawnIndex);
	});

	if (waveDirector.IsDeploymentComplete() && world.IsCleared())
	{
		if (hordeMode || waveDirector.HasMoreWaves())
		{
			if (hordeMode)
			{
				++hordeWavesSurvived;
				GrantHordeWaveUpgrade();
				hordeCurrentWave = hordeWavesSurvived + 1;
			}
			waveClearDelayActive = true;
			waveClearDelayRemaining = WaveClearDelayDuration;
			world.SetPlayerControlEnabled(false);
			BeginPlayerWaveTeleport();
			return;
		}

		CompleteCurrentLevel();
	}
}

void GameplayState::Render()
{
	auto& window{ GetContext().window };
	if (GetContext().settings.Get().graphics.postEffects)
	{
		postProcessor.Render(
			window,
			GetPresentationLevel().postProcess,
			effects.GetPostProcessState(),
			timeSlowdownVisualStrength,
			[this](sf::RenderTarget& target) { DrawScene(target); });
	}
	else
	{
		DrawScene(window);
	}

	waveIntro.Draw(window);
	levelIntro.Draw(window);

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
	else if (!levelIntro.IsActive() && !waveIntro.IsActive() &&
		!playerSpawnAnimating && !runMode &&
		!world.GetPlayerGamepadAimPoint())
		crosshair.Draw(GetContext().window);
	screenFade.Draw(GetContext().window);
}

void GameplayState::SpawnPlayerIfNeeded()
{
	if (!world.HasPlayer() && !session.IsGameOver())
	{
		world.SpawnPlayer(GetContext().assets, input);
		world.SetPlayerFiringEnabled(!runMode);
	}
}

void GameplayState::SpawnConfiguredEnemy(
	const GameplayData::SpawnGroup& spawn,
	std::size_t spawnIndex,
	bool materialize)
{
	using Kind = GameplayData::EnemyKind;
	std::unique_ptr<Entity> entity;
	bool edgeSpawn{ false };

	switch (spawn.kind)
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
	case Kind::Spinner:
		entity = std::make_unique<Spinner>(GetContext().assets, world);
		edgeSpawn = true;
		break;
	case Kind::MissileCarrier:
		entity = std::make_unique<MissileCarrier>(GetContext().assets, world);
		edgeSpawn = true;
		break;
	case Kind::LaserTurret:
		entity = std::make_unique<LaserTurret>(GetContext().assets, world);
		break;
	case Kind::ShooterStation:
		entity = std::make_unique<ShooterStation>(GetContext().assets, world);
		break;
	case Kind::ReflectorGunship:
		entity = std::make_unique<ReflectorGunship>(GetContext().assets, world);
		edgeSpawn = true;
		break;
	default:
		std::unreachable();
	}
	Enemy& enemy{ static_cast<Enemy&>(*entity) };
	const bool campaignSpawn{ !tutorialActive && !hordeMode && !runMode };
	const bool campaignEnemy{
		campaignSpawn &&
		spawn.kind != Kind::BigMeteor && spawn.kind != Kind::SmallMeteor };
	if (campaignEnemy)
	{
		const std::size_t ordinal{ campaignEnemySpawnOrdinal++ };
		if (const auto bonus{ campaignBonusDrops.find(ordinal) };
			bonus != campaignBonusDrops.end())
		{
			enemy.SetOrderedPickupDropCount(bonus->second);
		}
		if (const auto part{ campaignPartDrops.find(ordinal) };
			part != campaignPartDrops.end())
		{
			enemy.SetPartDropId(part->second);
		}
	}
	else if (!campaignSpawn)
	{
		enemy.SetPickupDrop(spawn.drop);
		if (spawnIndex < spawn.partIds.size())
			enemy.SetPartDropId(spawn.partIds[spawnIndex]);
	}

	if (auto* turret{ dynamic_cast<LaserTurret*>(entity.get()) })
	{
		const float inset{ turret->GetCollisionRadius() + 8.f };
		const float right{ static_cast<float>(world.GetWidth()) - inset };
		const float bottom{ static_cast<float>(world.GetHeight()) - inset };
		sf::Vector2f first;
		sf::Vector2f second;
		sf::Vector2f inward;
		if (spawnIndex == 0u)
			turretPathOffset = Random::Int(0, 3);
		switch ((turretPathOffset + static_cast<int>(spawnIndex)) % 4)
		{
		case 0: first = { inset, inset }; second = { right, inset }; inward = { 0.f, 1.f }; break;
		case 1: first = { right, inset }; second = { right, bottom }; inward = { -1.f, 0.f }; break;
		case 2: first = { right, bottom }; second = { inset, bottom }; inward = { 0.f, -1.f }; break;
		default: first = { inset, bottom }; second = { inset, inset }; inward = { 1.f, 0.f }; break;
		}
		turret->ConfigurePath(first, second, inward);
	}
	else if (auto* station{ dynamic_cast<ShooterStation*>(entity.get()) })
	{
		const float inset{ station->GetCollisionRadius() + 8.f };
		const float width{ static_cast<float>(world.GetWidth()) };
		const float height{ static_cast<float>(world.GetHeight()) };
		sf::Vector2f first;
		sf::Vector2f second;
		if (spawnIndex == 0u)
			stationPathOffset = Random::Int(0, 3);
		switch ((stationPathOffset + static_cast<int>(spawnIndex)) % 4)
		{
		case 0:
			first = { inset, height * 0.32f };
			second = { width - inset, height * 0.32f };
			break;
		case 1:
			first = { width - inset, height * 0.68f };
			second = { inset, height * 0.68f };
			break;
		case 2:
			first = { width * 0.32f, inset };
			second = { width * 0.32f, height - inset };
			break;
		default:
			first = { width * 0.68f, height - inset };
			second = { width * 0.68f, inset };
			break;
		}
		station->ConfigurePath(first, second);
	}
	else
	{
		entity->SetPosition(edgeSpawn ? GetSafeEdgeSpawnPosition() : GetSafeSpawnPosition());
		if (spawn.kind == Kind::Shooter || spawn.kind == Kind::Spinner ||
			spawn.kind == Kind::MissileCarrier ||
			spawn.kind == Kind::ReflectorGunship)
		{
			const sf::Vector2f entryTarget{
				Random::Float(world.GetWidth() * 0.36f, world.GetWidth() * 0.64f),
				Random::Float(world.GetHeight() * 0.32f, world.GetHeight() * 0.68f) };
			if (auto* saucer{ dynamic_cast<Saucer*>(entity.get()) })
				saucer->ConfigureApproachTarget(entryTarget);
			else if (auto* spinner{ dynamic_cast<Spinner*>(entity.get()) })
				spinner->ConfigureApproachTarget(entryTarget);
			else if (auto* carrier{ dynamic_cast<MissileCarrier*>(entity.get()) })
				carrier->ConfigureApproachTarget(entryTarget);
			else if (auto* reflector{ dynamic_cast<ReflectorGunship*>(entity.get()) })
				reflector->ConfigureApproachTarget(entryTarget);
		}
	}
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
	levelIntro.Reset();
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
	case HighlightParts:
		if (hud) hud->HighlightParts(5.f);
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
	case SpawnPart:
	{
		const sf::Vector2f playerPosition{ world.GetPlayerPosition() };
		const float horizontalOffset{
			playerPosition.x + 180.f < static_cast<float>(world.GetWidth())
				? 180.f
				: -180.f };
		auto part{ std::make_unique<Part>(
			GetContext().assets, world, "TUTORIAL-PART") };
		part->SetPosition(playerPosition + sf::Vector2f{ horizontalOffset, 0.f });
		world.Spawn(std::move(part));
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
	GetContext().audio.PlayGameplayMusic(
		GetGameplayMusic(session.GetLevel()), true, 50.f);
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
		GetContext().audio.PlayGameplayMusic(
			GetGameplayMusic(session.GetLevel()), true, 100.f);
}

TutorialDirector::Snapshot GameplayState::GetTutorialSnapshot() const
{
	const World::Statistics& statistics{ world.GetStatistics() };
	return {
		world.GetPlayerPosition(),
		statistics.playerAttacksFired,
		statistics.bigMeteorsDestroyed,
		statistics.smallMeteorsDestroyed,
		statistics.shootersDestroyed,
		statistics.shieldPickupsCollected,
		session.GetDisplayedParts() };
}

void GameplayState::Reset()
{
	world.Clear();
	effects.Clear();
	session.Reset();
	gameOverScreen.Reset();
	resultScreen.Reset();
	levelIntro.Reset();
	gameplayTransition = GameplayTransition::None;
	playerSpawnElapsed = 0.f;
	playerTeleportElapsed = 0.f;
	waveMaterializationElapsed = 0.f;
	waveClearDelayRemaining = 0.f;
	timeSlowdownVisualStrength = 0.f;
	GetContext().audio.SetGameplayPitch(1.f);
	playerSpawnAnimating = false;
	playerTeleportAnimating = false;
	playerTeleportMoved = false;
	waveClearDelayActive = false;
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
	session.StartAtLevel(level);
	session.ConfigureParts(progress->partsBalance, progress->collectedPartIds);
}

void GameplayState::SaveCompletedLevel()
{
	CampaignProgress* progress{ GetContext().campaignSave.EditProgress() };
	if (progress == nullptr)
		return;

	const int completedLevel{ session.GetLevel() };
	progress->partsBalance = session.GetPartsBalance();
	progress->collectedPartIds.assign(
		session.GetCollectedPartIds().begin(), session.GetCollectedPartIds().end());
	std::ranges::sort(progress->collectedPartIds);
	if (std::ranges::find(progress->completedLevels, completedLevel) ==
		progress->completedLevels.end())
	{
		progress->completedLevels.push_back(completedLevel);
	}

	int& bestScore{ progress->levelBestScores[completedLevel] };
	bestScore = std::max(bestScore, session.GetLevelScore());
	static_cast<void>(GetContext().records.SubmitCampaignLevelScore(
		completedLevel, session.GetLevelScore()));
	if (selectedLevelRun && !selectedLevelAdvancesCampaign)
	{
		static_cast<void>(GetContext().campaignSave.Save());
		return;
	}
	progress->highestUnlockedLevel = std::max(
		progress->highestUnlockedLevel,
		std::min(completedLevel + 1, gameplayData.GetLevelCount()));
	progress->phase = CampaignPhase::AwaitingUpgrades;

	constexpr int FinalCampaignLevel{ 10 };
	if (completedLevel >= FinalCampaignLevel)
	{
		progress->campaignCompleted = true;
		progress->currentLevel = FinalCampaignLevel;
	}
	else if (completedLevel >= gameplayData.GetLevelCount())
	{
		progress->campaignCompleted = false;
		progress->currentLevel = gameplayData.GetLevelCount();
	}
	else
	{
		progress->currentLevel = completedLevel + 1;
	}

	static_cast<void>(GetContext().campaignSave.Save());
}

ResultScreen::Statistics GameplayState::FinalizeLevelStatistics()
{
	const auto& level{ gameplayData.GetLevel(session.GetLevel()) };
	const World::Statistics& worldStatistics{ world.GetStatistics() };
	const float armorRatio{ std::clamp(
		session.GetPlayerHealth().GetRatio(), 0.f, 1.f) };
	const float rawAccuracyRatio{ worldStatistics.playerAttacksFired > 0u
		? std::clamp(
			static_cast<float>(worldStatistics.playerAttacksHit) /
			static_cast<float>(worldStatistics.playerAttacksFired), 0.f, 1.f)
		: 0.f };
	const float targetAccuracyRatio{ level.targetAccuracyPercent / 100.f };

	ResultScreen::Statistics result;
	result.combatScore = session.GetLevelScore();
	result.armorPercent = static_cast<int>(std::lround(armorRatio * 100.f));
	result.armorBonus = armorRatio >= ArmorBonusThreshold ? ArmorBonusPoints : 0;
	result.attacksHit = worldStatistics.playerAttacksHit;
	result.attacksFired = worldStatistics.playerAttacksFired;
	result.accuracyPercent = static_cast<int>(std::lround(rawAccuracyRatio * 100.f));
	result.targetAccuracyPercent = static_cast<int>(
		std::lround(level.targetAccuracyPercent));
	result.accuracyBonus = rawAccuracyRatio >= targetAccuracyRatio
		? AccuracyBonusPoints
		: 0;
	result.partsTotal = static_cast<int>(level.partIds.size());
	result.partsCollected = static_cast<int>(std::ranges::count_if(
		level.partIds, [this](const std::string& id)
		{
			return session.IsPartCollected(id);
		}));
	result.partsBonus = result.partsTotal > 0 &&
		result.partsCollected == result.partsTotal
		? AllPartsBonusPoints
		: 0;
	result.completionSeconds = levelGameplayElapsed;
	const int completionBonus{
		result.armorBonus + result.accuracyBonus + result.partsBonus };
	session.AddScore(completionBonus);
	result.levelTotal = result.combatScore + completionBonus;
	return result;
}

void GameplayState::CompleteCurrentLevel()
{
	const ResultScreen::Statistics levelStatistics{ FinalizeLevelStatistics() };
	session.ClearTemporaryEffects();
	world.ClearPickups();
	if (hud)
		hud->Update(0.f);
	BeginLevelCompleteAudio();
	session.SetLevelComplete();

	if (selectedLevelRun && !selectedLevelAdvancesCampaign)
		resultScreen.Start(ResultScreen::Mode::LevelReplay,
			session.GetLevel(), levelStatistics);
	else if (session.GetLevel() >= gameplayData.GetLevelCount())
		resultScreen.Start(ResultScreen::Mode::ContentComplete,
			session.GetLevel(), levelStatistics);
	else
		resultScreen.Start(ResultScreen::Mode::LevelComplete,
			session.GetLevel(), levelStatistics);
}

#ifdef _DEBUG
void GameplayState::DebugCompleteCurrentLevel()
{
	constexpr int CompleteUpgradeTestBalance{ 36 };
	const auto& level{ gameplayData.GetLevel(session.GetLevel()) };
	for (const std::string& id : level.partIds)
		static_cast<void>(session.RecoverPart(id));
	session.DebugPreparePartsBalance(CompleteUpgradeTestBalance);

	world.StopActiveSounds();
	world.ClearProjectiles();
	CompleteCurrentLevel();
}
#endif

void GameplayState::BeginGameOver()
{
	world.StopActiveSounds();
	world.AddSound(Config::Sound::ShipExplosion);
	GetContext().audio.PauseGameplayMusic();
	if (hordeMode)
	{
		static_cast<void>(GetContext().records.SubmitHordeResult(
			hordeWavesSurvived, session.GetScore()));
		gameOverScreen.StartHorde(session.GetScore(), hordeWavesSurvived);
	}
	else if (runMode)
	{
		const int survivedSeconds{ static_cast<int>(std::floor(runElapsed)) };
		static_cast<void>(GetContext().records.SubmitRunSeconds(survivedSeconds));
		gameOverScreen.StartRun(
			survivedSeconds,
			GetContext().records.Get().runSeconds);
	}
	else
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
	if (action == ResultScreen::Action::Restart)
	{
		world.StopActiveSounds();
		session.DiscardRecoveredParts();
		gameplayTransition = GameplayTransition::RestartLevel;
		screenFade.StartFadeOut(GameplayFadeOutDuration);
		return;
	}
	session.AcceptRecoveredParts();
	SaveCompletedLevel();
	GetContext().gameplayLaunch.upgradesReturnToLevelSelect = false;
	if (action == ResultScreen::Action::MainMenu)
		gameplayTransition = GameplayTransition::MainMenu;
	else if (resultScreen.GetMode() == ResultScreen::Mode::LevelReplay)
	{
		GetContext().gameplayLaunch.upgradesReturnToLevelSelect = true;
		gameplayTransition = GameplayTransition::ShipUpgrades;
	}
	else
		gameplayTransition = resultScreen.GetMode() == ResultScreen::Mode::Victory
			? GameplayTransition::RestartGame
			: GameplayTransition::ShipUpgrades;
	screenFade.StartFadeOut(GameplayFadeOutDuration);
}

void GameplayState::RestartCurrentLevel()
{
	if (tutorialActive)
	{
		StartTutorial();
		GetContext().audio.ResumeGameplayMusic();
		return;
	}
	if (hordeMode)
	{
		StartHorde();
		if (hud)
			hud->Update(0.f);
		GetContext().audio.ResumeGameplayMusic();
		return;
	}
	if (runMode)
	{
		StartRun();
		GetContext().audio.ResumeGameplayMusic();
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
	GetContext().audio.ResumeGameplayMusic();
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
	if (hud)
		hud->Update(0.f);
}

void GameplayState::SpawnLevel()
{
	const auto& level{ gameplayData.GetLevel(session.GetLevel()) };
	PrepareCampaignRewards(level);
	levelGameplayElapsed = 0.f;
	waveClearDelayActive = false;
	waveClearDelayRemaining = 0.f;
	background.SetTheme(level.background, level.backgroundBrightness);
	GetContext().audio.PlayGameplayMusic(GetGameplayMusic(level.number));
	waveDirector.LoadLevel(level);
	StartNextWave(false, false);
	levelIntro.Start(level.number, level.title);
}

void GameplayState::PrepareCampaignRewards(
	const GameplayData::LevelConfig& level)
{
	using EnemyKind = GameplayData::EnemyKind;
	campaignBonusDrops.clear();
	campaignPartDrops.clear();
	campaignEnemySpawnOrdinal = 0u;

	std::size_t enemyOrdinal{ 0u };
	std::vector<std::vector<std::size_t>> waveCandidates(level.waves.size());
	for (std::size_t waveIndex{ 0u }; waveIndex < level.waves.size(); ++waveIndex)
	{
		const auto collectEnemies{ [&](const auto& groups)
		{
			for (const auto& group : groups)
			{
				if (group.kind == EnemyKind::BigMeteor ||
					group.kind == EnemyKind::SmallMeteor)
				{
					continue;
				}
				for (int index{ 0 }; index < group.count; ++index)
				{
					waveCandidates[waveIndex].push_back(enemyOrdinal);
					++enemyOrdinal;
				}
			}
		} };
		collectEnemies(level.waves[waveIndex].initialSpawns);
		collectEnemies(level.waves[waveIndex].scheduledSpawns);
	}

	std::vector<std::string> partIds;
	for (const std::string& id : level.partIds)
	{
		if (!session.IsPartCollected(id))
			partIds.push_back(id);
	}
	for (std::size_t index{ partIds.size() }; index > 1u; --index)
	{
		const std::size_t other{ static_cast<std::size_t>(Random::Int(
			0, static_cast<int>(index) - 1)) };
		std::swap(partIds[index - 1u], partIds[other]);
	}

	const std::vector<GameplayData::PickupKind> bonuses{
		BuildCampaignBonusSequence(level.number) };
	world.ConfigureCampaignPickupSequence(bonuses);

	std::array<int, 3> bonusQuota{
		level.number / 3,
		level.number / 3,
		level.number / 3 };
	if (level.number == 9)
		bonusQuota = { 4, 4, 4 };
	else if (level.number == 1)
		bonusQuota = { 0, 1, 0 };
	else
	{
		for (int index{ 0 }; index < level.number % 3; ++index)
			++bonusQuota[static_cast<std::size_t>(index)];
	}

	std::vector<std::size_t> partCandidates;
	for (std::size_t waveIndex{ 0u }; waveIndex < waveCandidates.size(); ++waveIndex)
	{
		auto& candidates{ waveCandidates[waveIndex] };
		for (std::size_t index{ candidates.size() }; index > 1u; --index)
		{
			const std::size_t other{ static_cast<std::size_t>(Random::Int(
				0, static_cast<int>(index) - 1)) };
			std::swap(candidates[index - 1u], candidates[other]);
		}
		const std::size_t quota{ waveIndex < bonusQuota.size()
			? static_cast<std::size_t>(bonusQuota[waveIndex])
			: 0u };
		if (candidates.empty() && quota > 0u)
			return;
		for (std::size_t index{ 0u }; index < quota; ++index)
			++campaignBonusDrops[candidates[index % candidates.size()]];
		for (const std::size_t ordinal : candidates)
		{
			if (ordinal != 0u && !campaignBonusDrops.contains(ordinal))
				partCandidates.push_back(ordinal);
		}
	}
	if (partCandidates.size() < partIds.size())
	{
		return;
	}
	for (std::size_t index{ partCandidates.size() }; index > 1u; --index)
	{
		const std::size_t other{ static_cast<std::size_t>(Random::Int(
			0, static_cast<int>(index) - 1)) };
		std::swap(partCandidates[index - 1u], partCandidates[other]);
	}
	std::vector<std::size_t> partSlots(
		partCandidates.begin(), partCandidates.begin() +
		static_cast<std::ptrdiff_t>(partIds.size()));
	for (std::size_t index{ 0u }; index < partIds.size(); ++index)
		campaignPartDrops.emplace(partSlots[index], std::move(partIds[index]));
}

std::vector<GameplayData::PickupKind>
GameplayState::BuildCampaignBonusSequence(int levelNumber) const
{
	using Kind = GameplayData::PickupKind;
	constexpr std::array<Kind, 7> introductionOrder{
		Kind::Health,
		Kind::Shield,
		Kind::HomingBullets,
		Kind::TripleShot,
		Kind::TimeSlowdown,
		Kind::Laser,
		Kind::HelperBot };
	if (levelNumber == 9)
	{
		return {
			Kind::Health, Kind::Shield, Kind::HomingBullets, Kind::TripleShot,
			Kind::Health, Kind::TimeSlowdown, Kind::Laser, Kind::HelperBot,
			Kind::Health, Kind::Shield, Kind::HomingBullets, Kind::TripleShot };
	}

	const int count{ std::max(0, levelNumber) };
	std::vector<Kind> result;
	result.reserve(static_cast<std::size_t>(count));
	if (levelNumber <= static_cast<int>(introductionOrder.size()))
	{
		if (levelNumber > 0)
			result.push_back(introductionOrder[static_cast<std::size_t>(levelNumber - 1)]);
		for (int index{ 0 }; index < levelNumber - 1; ++index)
			result.push_back(introductionOrder[static_cast<std::size_t>(index)]);
		return result;
	}

	const std::size_t start{ static_cast<std::size_t>(
		(levelNumber - 1) % static_cast<int>(introductionOrder.size())) };
	for (int index{ 0 }; index < count; ++index)
	{
		result.push_back(introductionOrder[
			(start + static_cast<std::size_t>(index)) % introductionOrder.size()]);
	}
	return result;
}

void GameplayState::StartHorde()
{
	world.Clear();
	effects.Clear();
	session.ConfigureOneHitMode(false);
	session.ConfigureUpgrades({});
	session.ConfigurePlayerHealth(gameplayData.GetPlayer().maximumHealth);
	session.ConfigureParts(0, {});
	session.Reset();
	gameOverScreen.Reset();
	resultScreen.Reset();
	levelIntro.Reset();
	gameplayTransition = GameplayTransition::None;
	levelGameplayElapsed = 0.f;
	waveClearDelayActive = false;
	waveClearDelayRemaining = 0.f;
	playerSpawnAnimating = false;
	playerTeleportAnimating = false;
	playerTeleportMoved = false;
	materializingEnemies.clear();
	hordeCurrentWave = 1;
	hordeWavesSurvived = 0;
	hordeHelperAvailable = true;
	hordeBonusBag.clear();

	const HordeBackground& selectedBackground{ HordeBackgrounds[
		static_cast<std::size_t>(Random::Int(
			0, static_cast<int>(HordeBackgrounds.size()) - 1))] };
	background.SetTheme(selectedBackground.theme, selectedBackground.brightness);
	GetContext().audio.PlayGameplayMusic(Config::Music::GameplayBackground3);
	if (hud)
		hud->SetPartsVisible(false);
	StartNextHordeWave(false, false);
	levelIntro.StartMode(
		"HORDE MODE",
		"SURVIVE AS MANY WAVES AS POSSIBLE");
}

void GameplayState::StartRun()
{
	world.Clear();
	effects.Clear();
	session.ConfigureUpgrades({});
	session.ConfigurePlayerHealth(gameplayData.GetPlayer().maximumHealth);
	session.ConfigureParts(0, {});
	session.Reset();
	session.ConfigureOneHitMode(true);
	gameOverScreen.Reset();
	resultScreen.Reset();
	levelIntro.Reset();
	gameplayTransition = GameplayTransition::None;
	levelGameplayElapsed = 0.f;
	waveClearDelayActive = false;
	waveClearDelayRemaining = 0.f;
	playerSpawnAnimating = false;
	playerTeleportAnimating = false;
	playerTeleportMoved = false;
	materializingEnemies.clear();
	runElapsed = 0.f;
	runAsteroidTimer = RunAsteroidInterval;
	runEnemyTimer = RunEnemyInterval;
	runAsteroidsSpawned = 0;
	runShootersSpawned = 0;
	runLaserTurretSpawned = false;
	runReflectorSpawned = false;

	const auto& presentation{ gameplayData.GetLevel(3) };
	background.SetTheme(presentation.background, presentation.backgroundBrightness);
	GetContext().audio.PlayGameplayMusic(Config::Music::GameplayBackground2);
	if (hud)
	{
		hud->SetRunMode(true);
		hud->SetPartsVisible(false);
		hud->SetSurvivalTime(0.f);
		hud->Update(0.f);
	}
	levelIntro.StartMode(
		"RUN MODE",
		"KEEP MOVING. ONE HIT ENDS THE RUN");
}

void GameplayState::UpdateRun(float deltaTime)
{
	runElapsed += deltaTime;
	runAsteroidTimer -= deltaTime;
	runEnemyTimer -= deltaTime;

	while (runAsteroidTimer <= 0.f && runAsteroidsSpawned < RunAsteroidLimit)
	{
		SpawnRunAsteroid();
		++runAsteroidsSpawned;
		runAsteroidTimer += RunAsteroidInterval;
	}
	while (runEnemyTimer <= 0.f && runShootersSpawned < RunShooterLimit)
	{
		SpawnRunEnemy(GameplayData::EnemyKind::Shooter);
		++runShootersSpawned;
		runEnemyTimer += RunEnemyInterval;
	}
	if (!runLaserTurretSpawned && runElapsed >= 40.f)
	{
		SpawnRunEnemy(GameplayData::EnemyKind::LaserTurret);
		runLaserTurretSpawned = true;
	}
	if (!runReflectorSpawned && runElapsed >= 60.f)
	{
		SpawnRunEnemy(GameplayData::EnemyKind::ReflectorGunship);
		runReflectorSpawned = true;
	}
	world.CommitPendingEntities();
}

void GameplayState::SpawnRunAsteroid()
{
	auto meteor{ std::make_unique<Meteor>(
		GetContext().assets,
		world,
		Meteor::Size::Big) };
	meteor->SetPosition(GetSafeEdgeSpawnPosition());
	const float angle{ Random::Float(0.f, 2.f * std::numbers::pi_v<float>) };
	const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };
	const float speed{ Random::Float(80.f, 190.f) };
	meteor->SetVelocity(direction * speed);
	world.Spawn(std::move(meteor));
}

void GameplayState::SpawnRunEnemy(GameplayData::EnemyKind kind)
{
	GameplayData::SpawnGroup spawn;
	spawn.count = 1;
	spawn.kind = kind;
	SpawnConfiguredEnemy(spawn, 0u);
}

void GameplayState::StartNextHordeWave(
	bool materializeInitialSpawns,
	bool startWaveIntro)
{
	hordeLevel = GetPresentationLevel();
	hordeLevel.waves.clear();
	hordeLevel.waves.push_back(BuildHordeWave());
	waveDirector.LoadLevel(hordeLevel);
	StartNextWave(materializeInitialSpawns, false);
	if (startWaveIntro)
		waveIntro.Start(hordeCurrentWave);
}

GameplayData::WaveConfig GameplayState::BuildHordeWave()
{
	using EnemyKind = GameplayData::EnemyKind;
	GameplayData::WaveConfig wave;

	GameplayData::SpawnGroup::PickupDropConfig bonusDrop;
	bonusDrop.chance = 1.f;
	bonusDrop.pool.push_back({ TakeNextHordeBonus(), 1.f });

	GameplayData::SpawnGroup rewardMeteor;
	rewardMeteor.kind = EnemyKind::BigMeteor;
	rewardMeteor.count = 1;
	rewardMeteor.drop = std::move(bonusDrop);
	wave.initialSpawns.push_back(std::move(rewardMeteor));

	const int asteroidCount{ std::min(hordeCurrentWave, 10) };
	if (asteroidCount >= 2)
	{
		GameplayData::SpawnGroup additionalMeteors;
		additionalMeteors.kind = EnemyKind::BigMeteor;
		additionalMeteors.count = asteroidCount - 1;
		wave.initialSpawns.push_back(std::move(additionalMeteors));
	}

	const auto scheduleOne{ [&wave](EnemyKind kind, float delay)
	{
		GameplayData::WaveConfig::ScheduledSpawn spawn;
		spawn.kind = kind;
		spawn.count = 1;
		spawn.delay = delay;
		wave.scheduledSpawns.push_back(std::move(spawn));
	} };
	const auto scheduleMany{ [&scheduleOne](EnemyKind kind, int count)
	{
		for (int index{ 0 }; index < count; ++index)
			scheduleOne(kind, 2.f);
	} };

	if (hordeCurrentWave == 1)
	{
		scheduleMany(EnemyKind::Kamikaze, 3);
		return wave;
	}
	if (hordeCurrentWave == 2)
	{
		scheduleMany(EnemyKind::Shooter, 3);
		return wave;
	}

	// From Wave 3 onward, every previously introduced enemy gains one unit
	// per wave. A new enemy family joins each wave until the full roster is active.
	scheduleMany(EnemyKind::Kamikaze, hordeCurrentWave + 1);
	scheduleMany(EnemyKind::Shooter, hordeCurrentWave + 1);
	scheduleMany(EnemyKind::Spinner, hordeCurrentWave - 2);
	if (hordeCurrentWave >= 4)
		scheduleMany(EnemyKind::MissileCarrier, hordeCurrentWave - 3);
	if (hordeCurrentWave >= 5)
		scheduleMany(EnemyKind::LaserTurret, hordeCurrentWave - 4);
	if (hordeCurrentWave >= 6)
		scheduleMany(EnemyKind::ShooterStation, hordeCurrentWave - 5);
	if (hordeCurrentWave >= 7)
		scheduleMany(EnemyKind::ReflectorGunship, hordeCurrentWave - 6);
	return wave;
}

GameplayData::PickupKind GameplayState::TakeNextHordeBonus()
{
	using Kind = GameplayData::PickupKind;
	if (hordeBonusBag.empty())
	{
		hordeBonusBag = {
			Kind::Health,
			Kind::Shield,
			Kind::HomingBullets,
			Kind::TimeSlowdown,
			Kind::Laser,
			Kind::TripleShot };
		if (hordeHelperAvailable)
			hordeBonusBag.push_back(Kind::HelperBot);
	}

	const std::size_t index{ static_cast<std::size_t>(Random::Int(
		0, static_cast<int>(hordeBonusBag.size()) - 1)) };
	const Kind result{ hordeBonusBag[index] };
	hordeBonusBag.erase(hordeBonusBag.begin() + static_cast<std::ptrdiff_t>(index));
	if (result == Kind::HelperBot)
		hordeHelperAvailable = false;
	return result;
}

void GameplayState::GrantHordeWaveUpgrade()
{
	ShipUpgradeRanks ranks{ session.GetUpgradeRanks() };
	const int rewardIndex{ (hordeWavesSurvived - 1) % 4 };
	if (rewardIndex == 0)
		++ranks.armor;
	else if (rewardIndex == 1)
		++ranks.fireRate;
	else if (rewardIndex == 2)
		++ranks.engines;
	else
		++ranks.bonusDuration;

	const int previousMaximum{ session.GetPlayerHealth().GetMaximum() };
	session.ConfigureUpgrades(ranks, false);
	if (rewardIndex != 0)
		return;

	const int upgradedMaximum{ static_cast<int>(std::lround(
		static_cast<float>(gameplayData.GetPlayer().maximumHealth) *
		session.GetArmorMultiplier())) };
	Health& health{ session.GetPlayerHealth() };
	health.SetMaximum(upgradedMaximum, false);
	static_cast<void>(health.Restore(upgradedMaximum - previousMaximum));
}

const GameplayData::LevelConfig& GameplayState::GetPresentationLevel() const
{
	return gameplayData.GetLevel(
		hordeMode ? 9 : (runMode ? 3 : session.GetLevel()));
}

void GameplayState::StartNextWave(
	bool materializeInitialSpawns,
	bool startWaveIntro)
{
	waveClearDelayActive = false;
	waveClearDelayRemaining = 0.f;
	materializingEnemies.clear();
	waveMaterializationElapsed = 0.f;
	static_cast<void>(waveDirector.StartNextWave([this, materializeInitialSpawns](
		const GameplayData::SpawnGroup& spawn, std::size_t spawnIndex)
	{
		SpawnConfiguredEnemy(spawn, spawnIndex, materializeInitialSpawns);
	}));
	world.CommitPendingEntities();
	if (startWaveIntro)
		waveIntro.Start(
			waveDirector.GetCurrentWaveNumber(),
			!hordeMode && !waveDirector.HasMoreWaves());
}

void GameplayState::FinishWaveIntro()
{
	world.SetPlayerControlEnabled(true);
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
	{
		playerSpawnAnimating = false;
	}
}

void GameplayState::BeginPlayerWaveTeleport()
{
	if (!world.HasPlayer())
		return;
	playerTeleportElapsed = 0.f;
	playerTeleportAnimating = true;
	playerTeleportMoved = false;
	world.AddEffectEvent({
		World::EffectEventType::PlayerTeleport,
		world.GetPlayerPosition(), {}, 0.85f });
}

void GameplayState::UpdatePlayerWaveTeleport(float deltaTime)
{
	if (!playerTeleportAnimating)
		return;

	playerTeleportElapsed = std::min(
		PlayerTeleportDuration, playerTeleportElapsed + deltaTime);
	if (playerTeleportElapsed < PlayerTeleportMoveTime)
	{
		const float progress{ playerTeleportElapsed / PlayerTeleportMoveTime };
		world.SetPlayerSpawnPresentation(1.f - progress);
	}
	else
	{
		if (!playerTeleportMoved)
		{
			world.TeleportPlayerToCenter();
			world.AddEffectEvent({
				World::EffectEventType::PlayerTeleport,
				world.GetPlayerPosition(), {}, 1.15f });
			playerTeleportMoved = true;
		}
		const float progress{ std::clamp(
			(playerTeleportElapsed - PlayerTeleportMoveTime) /
			(PlayerTeleportDuration - PlayerTeleportMoveTime), 0.f, 1.f) };
		world.SetPlayerSpawnPresentation(progress);
	}

	if (playerTeleportElapsed >= PlayerTeleportDuration)
	{
		world.TeleportPlayerToCenter();
		world.SetPlayerSpawnPresentation(1.f);
		playerTeleportAnimating = false;
	}
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

void GameplayState::UpdateTimeSlowdownPresentation(float deltaTime)
{
	const float targetStrength{
		session.IsPlaying() && session.IsTimeSlowdownActive() && !waveIntro.IsActive()
			? 1.f
			: 0.f };
	const float maximumChange{ TimeSlowdownFadeSpeed * deltaTime };
	if (timeSlowdownVisualStrength < targetStrength)
		timeSlowdownVisualStrength = std::min(
			targetStrength, timeSlowdownVisualStrength + maximumChange);
	else
		timeSlowdownVisualStrength = std::max(
			targetStrength, timeSlowdownVisualStrength - maximumChange);

	const float targetPitch{ gameplayData.GetPickups().timeSlowdownAudioPitch };
	GetContext().audio.SetGameplayPitch(std::lerp(
		1.f, targetPitch, timeSlowdownVisualStrength));
}

float GameplayState::GetWorldTimeScale() const noexcept
{
	return session.IsPlaying() && session.IsTimeSlowdownActive()
		? gameplayData.GetPickups().timeSlowdownWorldScale
		: 1.f;
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
