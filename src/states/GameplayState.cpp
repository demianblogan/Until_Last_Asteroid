#include "GameplayState.h"

#include <algorithm>
#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include "assets/AssetStore.h"
#include "entities/Meteor.h"
#include "entities/Saucer.h"
#include "ui/HUD.h"
#include "utils/Random.h"

GameplayState::GameplayState(StateStack& stateStack, StateContext context)
	: State(stateStack, context)
	, input(actions)
	, world(
		static_cast<unsigned int>(context.logicalSize.x),
		static_cast<unsigned int>(context.logicalSize.y),
		context.assets,
		session)
{
	world.SetWindow(context.window);
	context.window.setMouseCursor(context.assets.GetCursor(Config::Cursor::GameplayCrosshair));

	hud.emplace(context.assets, session);

	SetupInput();
	SetupUI();
	Reset();

	sf::Music& gameplayMusic{ context.assets.Music().Get(Config::Music::GameplayTheme) };
	gameplayMusic.setLooping(true);
	if (gameplayMusic.getStatus() != sf::SoundSource::Status::Playing)
		gameplayMusic.play();
}

GameplayState::~GameplayState()
{
	GetContext().assets.Music().Get(Config::Music::GameplayTheme).stop();
}

GameplayState::LevelData GameplayState::CreateLevel(int level)
{
	LevelData data;

	switch (level)
	{
	case 1:
		data.initialMeteors = 3;
		break;

	case 2:
		data.initialMeteors = 2;
		data.waves.push_back({ 2.f, 2, 0.f, 0, true, false });
		break;

	case 3:
		data.initialMeteors = 3;
		data.waves.push_back({ 2.f, 3, 0.f, 0, false, true });
		break;

	case 4:
		data.initialMeteors = 4;
		data.waves.push_back({ 2.f, 3, 0.f, 0, true, true });
		break;

	case 5:
		data.initialMeteors = 5;
		data.initialShooters = 3;
		data.waves.push_back({ 1.f, 3, 0.f, 0, true, true });
		break;
	}

	return data;
}

void GameplayState::SetupInput()
{
	using enum Config::PlayerAction;
	using enum InputAction::TriggerType;
	using namespace sf::Keyboard;

	actions.AddBinding(Up, InputAction(Key::W, WhileHeld));
	actions.AddBinding(Left, InputAction(Key::A, WhileHeld));
	actions.AddBinding(Right, InputAction(Key::D, WhileHeld));
	actions.AddBinding(Down, InputAction(Key::S, WhileHeld));
}

void GameplayState::SetupUI()
{
	sf::Font& font = GetContext().assets.Fonts().Get(Config::Font::GUI);

	// EXIT
	exitHintText.emplace(font);
	exitHintText->setString("ESC - Pause Menu");
	exitHintText->setCharacterSize(25);
	exitHintText->setPosition({ 20.f, 20.f });

	// GAME OVER
	{
		sf::Text gameOver(font);
		gameOver.setString("GAME OVER");
		gameOver.setCharacterSize(100);
		CenterText(gameOver, GetContext().logicalSize.y * 0.4f);

		sf::Text restart(font);
		restart.setString("Press SPACE to restart");
		restart.setCharacterSize(50);
		CenterText(restart, GetContext().logicalSize.y * 0.6f);

		gameOverTexts = { gameOver, restart };
	}

	// LEVEL COMPLETE
	{
		sf::Text levelComplete(font);
		levelComplete.setString("LEVEL " + std::to_string(session.GetLevel()) + " COMPLETE");
		levelComplete.setCharacterSize(100);
		CenterText(levelComplete, GetContext().logicalSize.y * 0.4f);

		sf::Text next(font);
		next.setString("Press SPACE to continue");
		next.setCharacterSize(50);
		CenterText(next, GetContext().logicalSize.y * 0.6f);

		levelCompleteTexts = { levelComplete, next };
	}

	// WIN
	{
		sf::Text title(font);
		title.setString("YOU WIN!");
		title.setCharacterSize(100);
		CenterText(title, GetContext().logicalSize.y * 0.35f);

		sf::Text score(font);
		score.setString("FINAL SCORE: 0");
		score.setCharacterSize(50);
		CenterText(score, GetContext().logicalSize.y * 0.5f);

		sf::Text restart(font);
		restart.setString("Press SPACE to restart");
		restart.setCharacterSize(40);
		CenterText(restart, GetContext().logicalSize.y * 0.65f);

		winTexts = { title, score, restart };
	}
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
			if (session.IsGameOver())
			{
				Reset();
				return;
			}
			else if (session.IsLevelComplete())
			{
				NextLevel();
				return;
			}
			else if (session.IsWin())
			{
				Reset();
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
	if (session.IsWin() && winTexts.size() >= 2)
	{
		winTexts[1].setString("Score: " + std::to_string(session.GetScore()));
		CenterTextX(winTexts[1]);
	}

	if (!session.IsPlaying())
		return;

	SpawnPlayerIfNeeded();
	world.Update(dt);

	// A collision may have changed the state to GameOver. Do not let the
	// remaining level logic overwrite that terminal state.
	if (!session.IsPlaying())
		return;

	// ====================================================
	// LEVEL SPAWN SYSTEM
	// ====================================================
	for (SpawnWave& wave : currentLevel.waves)
	{
		if (wave.spawned >= wave.totalSpawns)
			continue;

		wave.timer += dt;

		if (wave.timer >= wave.interval)
		{
			wave.timer = 0.f;
			wave.spawned++;

			if (wave.spawnKamikaze)
			{
				auto saucer{ std::make_unique<Saucer>(GetContext().assets, world, Saucer::Mode::Kamikaze) };

				saucer->SetPosition(GetSafeEdgeSpawnPosition());
				world.Spawn(std::move(saucer));
				world.AddSound(Config::Sound::SaucerKamikazeSpawn);
			}

			if (wave.spawnShooter)
			{
				auto saucer{ std::make_unique<Saucer>(GetContext().assets, world, Saucer::Mode::Shooter) };

				saucer->SetPosition(GetSafeEdgeSpawnPosition());
				world.Spawn(std::move(saucer));
				world.AddSound(Config::Sound::SaucerShooterSpawn);
			}
		}
	}

	// ====================================================
	// LEVEL CHECKING
	// ====================================================
	const bool allWavesSpawned{ std::all_of(currentLevel.waves.begin(), currentLevel.waves.end(),
		[](const SpawnWave& wave)
		{
			return wave.spawned >= wave.totalSpawns;
		}) };

	if (allWavesSpawned && world.IsCleared())
	{
		if (session.GetLevel() >= 5)
		{
			session.SetWin();
		}
		else
		{
			session.SetLevelComplete();

			if (!levelCompleteTexts.empty())
			{
				levelCompleteTexts[0].setString("LEVEL " + std::to_string(session.GetLevel()) + " COMPLETE");
				CenterTextX(levelCompleteTexts[0]);
			}
		}

		return;
	}

	if (hud.has_value())
		hud->Update();
}

// --------------------------------------------------------
void GameplayState::Render()
{
	auto& window{ GetContext().window };

	if (session.IsGameOver())
	{
		for (sf::Text& text : gameOverTexts)
			window.draw(text);
	}
	else if (session.IsLevelComplete())
	{
		for (sf::Text& text : levelCompleteTexts)
			window.draw(text);
	}
	else if (session.IsWin())
	{
		for (sf::Text& text : winTexts)
			window.draw(text);
	}
	else
	{
		window.draw(world);

		if (hud)
			hud->Draw(window);
	}

	if (!session.IsPlaying() && exitHintText)
		window.draw(*exitHintText);
}

void GameplayState::SpawnPlayerIfNeeded()
{
	if (!world.HasPlayer() && !session.IsGameOver())
		world.SpawnPlayer(GetContext().assets, input);
}

void GameplayState::Reset()
{
	world.Clear();
	session.Reset();

	SpawnLevel();
}

void GameplayState::NextLevel()
{
	if (session.GetLevel() >= 5)
	{
		session.SetWin();
		return;
	}

	world.Clear();

	session.NextLevel();

	SpawnPlayerIfNeeded();
	SpawnLevel();
}

void GameplayState::SpawnLevel()
{
	currentLevel = CreateLevel(session.GetLevel());

	// --- METEORS ---
	for (int i{ 0 }; i < currentLevel.initialMeteors; i++)
	{
		auto meteor{ std::make_unique<Meteor>(GetContext().assets, world, Meteor::Size::Big) };

		meteor->SetPosition(GetSafeSpawnPosition());
		world.Spawn(std::move(meteor));
	}

	// --- SHOOTERS ---
	for (int i{ 0 }; i < currentLevel.initialShooters; i++)
	{
		auto saucer = std::make_unique<Saucer>(GetContext().assets, world, Saucer::Mode::Shooter);

		saucer->SetPosition(GetSafeEdgeSpawnPosition());
		world.Spawn(std::move(saucer));
	}
}

sf::Vector2f GameplayState::GetSafeSpawnPosition()
{
	static constexpr int MAXIMUM_ATTEMPS_TO_FIND_SPAWN_POSITION = 50;
	for (int i{ 0 }; i < MAXIMUM_ATTEMPS_TO_FIND_SPAWN_POSITION; i++)
	{
		sf::Vector2f position{ Random::Float(0.f, float(world.GetWidth())), Random::Float(0.f, float(world.GetHeight())) };

		if (!world.HasPlayer())
			return position;

		sf::Vector2f playerPos = world.GetPlayerPosition();

		float dx{ position.x - playerPos.x };
		float dy{ position.y - playerPos.y };

		float distSq{ dx * dx + dy * dy };

		if (distSq > SPAWN_SAFE_RADIUS * SPAWN_SAFE_RADIUS)
			return position;
	}

	return sf::Vector2f{ Random::Float(0.f, float(world.GetWidth())), Random::Float(0.f, float(world.GetHeight())) };
}

sf::Vector2f GameplayState::GetSafeEdgeSpawnPosition()
{
	auto spawnAtEdge = [&]()
		{
			float width{ float(world.GetWidth()) };
			float height{ float(world.GetHeight()) };
			int randomNumber{ Random::Int(0, 3) };

			switch (randomNumber)
			{
			case 0:
				return sf::Vector2f{ 0.f, Random::Float(0.f, height) };
			case 1:
				return sf::Vector2f{ width, Random::Float(0.f, height) };
			case 2:
				return sf::Vector2f{ Random::Float(0.f, width), 0.f };
			case 3:
				return sf::Vector2f{ Random::Float(0.f, width), height };
			default:
				std::unreachable();
			}
		};

	static constexpr int MAXIMUM_ATTEMPS_TO_FIND_SPAWN_POSITION = 50;
	for (int i{ 0 }; i < MAXIMUM_ATTEMPS_TO_FIND_SPAWN_POSITION; i++)
	{
		sf::Vector2f position = spawnAtEdge();

		if (!world.HasPlayer())
			return position;

		sf::Vector2f playerPos{ world.GetPlayerPosition() };

		float dx{ position.x - playerPos.x };
		float dy{ position.y - playerPos.y };

		float distSq{ dx * dx + dy * dy };

		if (distSq > SPAWN_SAFE_RADIUS * SPAWN_SAFE_RADIUS)
			return position;
	}

	return spawnAtEdge();
}

void GameplayState::CenterTextX(sf::Text& text)
{
	sf::FloatRect bounds{ text.getLocalBounds() };
	text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y });
	text.setPosition({ GetContext().logicalSize.x * 0.5f, text.getPosition().y });
}

void GameplayState::CenterText(sf::Text& text, float y)
{
	sf::FloatRect bounds = text.getLocalBounds();
	text.setOrigin({
		bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y
		});

	text.setPosition({ GetContext().logicalSize.x * 0.5f, y });
}
