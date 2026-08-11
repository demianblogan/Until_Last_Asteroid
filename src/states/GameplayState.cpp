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
#include "utils/Random.h"

namespace
{
	constexpr sf::Color CrosshairGlowColor{ 25, 220, 255 };
}

GameplayState::GameplayState(StateStack& stateStack, StateContext context)
	: State(stateStack, context)
	, gameplayData(context.assets.GetGameplayData())
	, input(actions)
	, background(context.assets, context.logicalSize)
	, effects(gameplayData.GetEffects())
	, world(static_cast<unsigned int>(context.logicalSize.x),
		static_cast<unsigned int>(context.logicalSize.y),
		context.assets, context.audio, session)
	, crosshair(context.assets, Config::Texture::GameplayCrosshair,
		{ 32.f, 32.f }, CrosshairGlowColor)
{
	world.SetWindow(context.window);
	context.window.setMouseCursorVisible(false);
	session.ConfigurePlayerHealth(gameplayData.GetPlayer().maximumHealth);
	hud.emplace(context.assets, session);
	SetupInput();
	SetupUI();
	Reset();
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

void GameplayState::SetupUI()
{
	sf::Font& font{ GetContext().assets.Fonts().Get(Config::Font::GUI) };
	exitHintText.emplace(font);
	exitHintText->setString("ESC - Pause Menu");
	exitHintText->setCharacterSize(25);
	exitHintText->setPosition({ 20.f, 20.f });

	sf::Text gameOver(font, "GAME OVER", 100);
	CenterText(gameOver, GetContext().logicalSize.y * 0.4f);
	sf::Text restart(font, "Press SPACE to restart", 50);
	CenterText(restart, GetContext().logicalSize.y * 0.6f);
	gameOverTexts = { gameOver, restart };

	sf::Text levelComplete(font, "LEVEL 1 COMPLETE", 100);
	CenterText(levelComplete, GetContext().logicalSize.y * 0.4f);
	sf::Text next(font, "Press SPACE to continue", 50);
	CenterText(next, GetContext().logicalSize.y * 0.6f);
	levelCompleteTexts = { levelComplete, next };

	sf::Text title(font, "YOU WIN!", 100);
	CenterText(title, GetContext().logicalSize.y * 0.35f);
	sf::Text score(font, "FINAL SCORE: 0", 50);
	CenterText(score, GetContext().logicalSize.y * 0.5f);
	sf::Text winRestart(font, "Press SPACE to restart", 40);
	CenterText(winRestart, GetContext().logicalSize.y * 0.65f);
	winTexts = { title, score, winRestart };
}

void GameplayState::HandleEvent(const sf::Event& event)
{
	if (event.is<sf::Event::FocusLost>() && session.IsPlaying())
	{
		OpenPauseMenu();
		return;
	}

	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		if (key->code == sf::Keyboard::Key::Escape)
		{
			OpenPauseMenu();
			return;
		}

		if (key->code == sf::Keyboard::Key::Space)
		{
			if (session.IsGameOver() || session.IsWin())
			{
				Reset();
				return;
			}
			if (session.IsLevelComplete())
			{
				NextLevel();
				return;
			}
		}
	}

	if (session.IsPlaying())
		world.HandlePlayerEvent(event);
}

void GameplayState::HandleRealtime()
{
	ResumeGameplaySounds();
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
	background.Update(dt);
	crosshair.Update(dt);
	if (session.IsWin() && winTexts.size() >= 2)
	{
		winTexts[1].setString("Score: " + std::to_string(session.GetScore()));
		CenterTextX(winTexts[1]);
	}

	if (!session.IsPlaying())
	{
		effects.Update(dt, world);
		return;
	}

	world.Update(dt);
	effects.Update(dt, world);
	if (hud)
		hud->Update(dt);
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
			session.SetWin();
		else
		{
			session.SetLevelComplete();
			levelCompleteTexts[0].setString("LEVEL " +
				std::to_string(session.GetLevel()) + " COMPLETE");
			CenterTextX(levelCompleteTexts[0]);
		}
	}
}

void GameplayState::Render()
{
	auto& window{ GetContext().window };
	window.draw(background);

	sf::RenderStates worldStates;
	worldStates.transform.translate(effects.GetCameraOffset());
	effects.DrawBehindEntities(window, worldStates);
	window.draw(world, worldStates);
	effects.DrawAboveEntities(window, worldStates);

	if (session.IsPlaying())
	{
		if (hud) hud->Draw(window);
	}
	else if (session.IsGameOver())
		for (const sf::Text& text : gameOverTexts) window.draw(text);
	else if (session.IsLevelComplete())
		for (const sf::Text& text : levelCompleteTexts) window.draw(text);
	else if (session.IsWin())
		for (const sf::Text& text : winTexts) window.draw(text);

	if (!session.IsPlaying() && exitHintText)
		window.draw(*exitHintText);
}

void GameplayState::RenderOverlay()
{
	crosshair.Draw(GetContext().window);
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
	SpawnLevel();
	if (hud) hud->Update(0.f);
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
	SpawnLevel();
}

void GameplayState::SpawnLevel()
{
	SpawnPlayerIfNeeded();
	currentWaves.clear();
	const auto& level{ gameplayData.GetLevel(session.GetLevel()) };
	background.SetTheme(level.background);
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

void GameplayState::CenterTextX(sf::Text& text)
{
	const sf::FloatRect bounds{ text.getLocalBounds() };
	text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y });
	text.setPosition({ GetContext().logicalSize.x * 0.5f, text.getPosition().y });
}

void GameplayState::CenterText(sf::Text& text, float y)
{
	const sf::FloatRect bounds{ text.getLocalBounds() };
	text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y });
	text.setPosition({ GetContext().logicalSize.x * 0.5f, y });
}
