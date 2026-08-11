#include "GameplayState.h"

#include <algorithm>
#include <utility>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "entities/Meteor.h"
#include "entities/Saucer.h"
#include "settings/SettingsManager.h"
#include "systems/GamepadManager.h"
#include "utils/Random.h"

namespace
{
	constexpr sf::Color CrosshairGlowColor{ 25, 220, 255 };
	constexpr float GameplayFadeInDuration{ 0.45f };
	constexpr float GameplayFadeOutDuration{ 0.38f };
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
{
	world.SetWindow(context.window);
	context.window.setMouseCursorVisible(false);
	session.ConfigurePlayerHealth(gameplayData.GetPlayer().maximumHealth);
	hud.emplace(context.assets, session);
	SetupInput();
	Reset();
	screenFade.StartFadeIn(GameplayFadeInDuration);
	context.audio.PlayMusic(Config::Music::GameplayBackground1);
}

GameplayState::~GameplayState()
{
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

	if (session.IsPlaying())
		world.HandlePlayerEvent(event);
}

void GameplayState::HandleRealtime()
{
	if (session.IsPlaying())
		ResumeGameplaySounds();
	if (screenFade.IsActive() || gameOverScreen.IsActive() || resultScreen.IsActive())
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
				Reset();
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
	if (!session.IsPlaying())
	{
		effects.Update(dt, world,
			GetContext().settings.Get().gameplay.screenShake,
			GetContext().settings.Get().gameplay.showScorePopups);
		return;
	}

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

	for (RuntimeWave& wave : currentWaves)
	{
		if (wave.repetitionsSpawned >= wave.config.repetitions)
			continue;

		wave.timer += dt;
		if (wave.timer < wave.config.interval)
			continue;

		wave.timer -= wave.config.interval;
		++wave.repetitionsSpawned;
		for (GameplayData::EnemyKind kind : wave.config.spawns)
			SpawnConfiguredEnemy(kind);
	}

	const bool allWavesSpawned{ std::ranges::all_of(currentWaves,
		[](const RuntimeWave& wave)
		{
			return wave.repetitionsSpawned >= wave.config.repetitions;
		}) };

	if (allWavesSpawned && world.IsCleared())
	{
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

	if (session.IsPlaying())
	{
		if (hud) hud->Draw(window);
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
	else if (!world.GetPlayerGamepadAimPoint())
		crosshair.Draw(GetContext().window);
	screenFade.Draw(GetContext().window);
}

void GameplayState::SpawnPlayerIfNeeded()
{
	if (!world.HasPlayer() && !session.IsGameOver())
		world.SpawnPlayer(GetContext().assets, input);
}

void GameplayState::SpawnConfiguredEnemy(GameplayData::EnemyKind kind)
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
	world.Spawn(std::move(entity));
}

void GameplayState::Reset()
{
	world.Clear();
	effects.Clear();
	session.Reset();
	gameOverScreen.Reset();
	resultScreen.Reset();
	gameplayTransition = GameplayTransition::None;
	SpawnLevel();
	if (hud) hud->Update(0.f);
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
	SpawnPlayerIfNeeded();
	currentWaves.clear();
	const auto& level{ gameplayData.GetLevel(session.GetLevel()) };
	background.SetTheme(level.background, level.backgroundBrightness);
	for (const auto& group : level.initialSpawns)
		for (int i{ 0 }; i < group.count; ++i)
			SpawnConfiguredEnemy(group.kind);
	for (const auto& wave : level.waves)
		currentWaves.push_back({ wave });
}

sf::Vector2f GameplayState::GetSafeSpawnPosition()
{
	constexpr int MaximumAttempts{ 50 };
	for (int i{ 0 }; i < MaximumAttempts; ++i)
	{
		const sf::Vector2f position{ Random::Float(0.f, static_cast<float>(world.GetWidth())),
			Random::Float(0.f, static_cast<float>(world.GetHeight())) };
		if (!world.HasPlayer()) return position;
		const sf::Vector2f delta{ position - world.GetPlayerPosition() };
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
