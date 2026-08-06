#include "Game.h"

#include <algorithm>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>
#include "entities/Meteor.h"
#include "entities/Saucer.h"
#include "ui/HUD.h"
#include "utils/Random.h"

Game::Game()
	: window(sf::VideoMode::getDesktopMode(), "Until last asteroid", sf::State::Fullscreen)
	, world(static_cast<unsigned int>(LOGICAL_SIZE.x), static_cast<unsigned int>(LOGICAL_SIZE.y), assets, gameState)
	, input(actions)
{
	world.SetWindow(window);

	sf::View view(sf::FloatRect({ 0.f, 0.f }, LOGICAL_SIZE));
	window.setView(GetLetterboxView(view, window.getSize().x, window.getSize().y));

	assets.Initialize();

	window.setMouseCursor(assets.GetCursor(Config::Cursor::Crosshair));
	window.setVerticalSyncEnabled(true);

	hud.emplace(assets, gameState);

	SetupInput();
	SetupUI();
	Reset();

	sf::Music& backgroundMusic = assets.Music().Get(Config::Music::BackgroundTheme);
	backgroundMusic.setLooping(true);
	backgroundMusic.play();
}

void Game::Run()
{
	sf::Clock clock;

	while (window.isOpen())
	{
		const float deltaTime{ std::min(clock.restart().asSeconds(), MAX_FRAME_TIME) };

		ProcessEvents();
		Update(deltaTime);
		Render();
	}
}

Game::LevelData Game::CreateLevel(int level)
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

void Game::SetupInput()
{
	using enum Config::PlayerAction;
	using enum InputAction::TriggerType;
	using namespace sf::Keyboard;

	actions.AddBinding(Up, InputAction(Key::W, WhileHeld));
	actions.AddBinding(Left, InputAction(Key::A, WhileHeld));
	actions.AddBinding(Right, InputAction(Key::D, WhileHeld));
	actions.AddBinding(Down, InputAction(Key::S, WhileHeld));
	actions.AddBinding(Shoot, InputAction(Key::Space, OnPress));
}

void Game::SetupUI()
{
	sf::Font& font = assets.Fonts().Get(Config::Font::GUI);

	// EXIT
	exitHintText.emplace(font);
	exitHintText->setString("ESC - Exit");
	exitHintText->setCharacterSize(25);
	exitHintText->setPosition({ 20.f, 20.f });

	// START
	{
		sf::Text title(font);
		title.setString("UNTIL LAST ASTEROID");
		title.setCharacterSize(100);
		CenterText(title, LOGICAL_SIZE.y * 0.10f);

		sf::Text controls(font);
		controls.setString(
			"W A S D - Move\n"
			"Mouse - Aim\n"
			"LMB - Shoot"
		);
		controls.setCharacterSize(40);
		CenterText(controls, LOGICAL_SIZE.y * 0.4f);

		sf::Text start(font);
		start.setString("Press SPACE to start");
		start.setCharacterSize(50);
		CenterText(start, LOGICAL_SIZE.y * 0.75f);

		startScreenTexts = { title, controls, start };
	}

	// GAME OVER
	{
		sf::Text gameOver(font);
		gameOver.setString("GAME OVER");
		gameOver.setCharacterSize(100);
		CenterText(gameOver, LOGICAL_SIZE.y * 0.4f);

		sf::Text restart(font);
		restart.setString("Press SPACE to restart");
		restart.setCharacterSize(50);
		CenterText(restart, LOGICAL_SIZE.y * 0.6f);

		gameOverTexts = { gameOver, restart };
	}

	// LEVEL COMPLETE
	{
		sf::Text levelComplete(font);
		levelComplete.setString("LEVEL " + std::to_string(gameState.GetLevel()) + " COMPLETE");
		levelComplete.setCharacterSize(100);
		CenterText(levelComplete, LOGICAL_SIZE.y * 0.4f);

		sf::Text next(font);
		next.setString("Press SPACE to continue");
		next.setCharacterSize(50);
		CenterText(next, LOGICAL_SIZE.y * 0.6f);

		levelCompleteTexts = { levelComplete, next };
	}

	// WIN
	{
		sf::Text title(font);
		title.setString("YOU WIN!");
		title.setCharacterSize(100);
		CenterText(title, LOGICAL_SIZE.y * 0.35f);

		sf::Text score(font);
		score.setString("FINAL SCORE: 0");
		score.setCharacterSize(50);
		CenterText(score, LOGICAL_SIZE.y * 0.5f);

		sf::Text restart(font);
		restart.setString("Press SPACE to restart");
		restart.setCharacterSize(40);
		CenterText(restart, LOGICAL_SIZE.y * 0.65f);

		winTexts = { title, score, restart };
	}
}

void Game::ProcessEvents()
{
	while (std::optional<sf::Event> event{ window.pollEvent() })
	{
		if (auto* resized{ event->getIf<sf::Event::Resized>() })
		{
			sf::View view(sf::FloatRect({ 0.f, 0.f }, LOGICAL_SIZE));
			window.setView(GetLetterboxView(view, resized->size.x, resized->size.y));
		}

		if (event->is<sf::Event::Closed>())
			window.close();

		if (auto* key{ event->getIf<sf::Event::KeyPressed>() })
		{
			if (key->code == sf::Keyboard::Key::Escape)
			{
				window.close();
				return;
			}

			if (key->code == sf::Keyboard::Key::Space)
			{
				if (gameState.IsStart())
				{
					gameState.StartGame();
					return;
				}
				else if (gameState.IsGameOver())
				{
					Reset();
					gameState.StartGame();
					return;
				}
				else if (gameState.IsLevelComplete())
				{
					NextLevel();
					gameState.StartGame();
					return;
				}
				else if (gameState.IsWin())
				{
					Reset();
					gameState.StartGame();
					return;
				}
			}
		}

		if (gameState.IsPlaying())
			world.HandlePlayerEvent(*event);
	}

	if (gameState.IsPlaying())
		world.HandlePlayerRealtime();
}

void Game::Update(float dt)
{
	gameTime += dt;

	if (gameState.IsWin() && winTexts.size() >= 2)
	{
		winTexts[1].setString("Score: " + std::to_string(gameState.GetScore()));
		CenterTextX(winTexts[1]);
	}

	if (!gameState.IsPlaying())
		return;

	SpawnPlayerIfNeeded();
	world.Update(dt);


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
				auto saucer{ std::make_unique<Saucer>(assets, world, Saucer::Mode::Kamikaze) };

				saucer->SetPosition(GetSafeEdgeSpawnPosition());
				world.Spawn(std::move(saucer));
				world.AddSound(Config::Sound::SaucerKamikazeSpawn);
			}

			if (wave.spawnShooter)
			{
				auto saucer{ std::make_unique<Saucer>(assets, world, Saucer::Mode::Shooter) };

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
		if (gameState.GetLevel() >= 5)
		{
			gameState.SetWin();
		}
		else
		{
			gameState.SetLevelComplete();

			if (!levelCompleteTexts.empty())
			{
				levelCompleteTexts[0].setString("LEVEL " + std::to_string(gameState.GetLevel()) + " COMPLETE");
				CenterTextX(levelCompleteTexts[0]);
			}
		}

		return;
	}

	if (hud.has_value())
		hud->Update();
}

// --------------------------------------------------------
void Game::Render()
{
	window.clear();

	if (gameState.IsStart())
	{
		for (sf::Text& text : startScreenTexts)
			window.draw(text);
	}
	else if (gameState.IsGameOver())
	{
		for (sf::Text& text : gameOverTexts)
			window.draw(text);
	}
	else if (gameState.IsLevelComplete())
	{
		for (sf::Text& text : levelCompleteTexts)
			window.draw(text);
	}
	else if (gameState.IsWin())
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

	if (!gameState.IsPlaying() && exitHintText)
		window.draw(*exitHintText);

	window.display();
}

void Game::SpawnPlayerIfNeeded()
{
	if (!world.HasPlayer() && !gameState.IsGameOver())
		world.SpawnPlayer(assets, input);
}

void Game::Reset()
{
	gameTime = 0.f;

	world.Clear();
	gameState.Reset();

	SpawnLevel();
}

void Game::NextLevel()
{
	if (gameState.GetLevel() >= 5)
	{
		gameState.SetWin();
		return;
	}

	world.Clear();

	gameState.NextLevel();

	SpawnPlayerIfNeeded();
	SpawnLevel();
}

void Game::SpawnLevel()
{
	currentLevel = CreateLevel(gameState.GetLevel());

	// --- METEORS ---
	for (int i{ 0 }; i < currentLevel.initialMeteors; i++)
	{
		auto meteor{ std::make_unique<Meteor>(assets, world, Meteor::Size::Big) };

		meteor->SetPosition(GetSafeSpawnPosition());
		world.Spawn(std::move(meteor));
	}

	// --- SHOOTERS ---
	for (int i{ 0 }; i < currentLevel.initialShooters; i++)
	{
		auto saucer = std::make_unique<Saucer>(assets, world, Saucer::Mode::Shooter);

		saucer->SetPosition(GetSafeEdgeSpawnPosition());
		world.Spawn(std::move(saucer));
	}
}

sf::Vector2f Game::GetSafeSpawnPosition()
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

sf::Vector2f Game::GetSafeEdgeSpawnPosition()
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

void Game::CenterTextBlock(std::vector<sf::Text>& texts, float startY, float spacing)
{
	float totalHeight{ 0.f };

	for (sf::Text& text : texts)
	{
		sf::FloatRect bounds{ text.getLocalBounds() };
		totalHeight += bounds.size.y;
	}

	totalHeight += spacing * (texts.size() - 1);

	float y{ startY - totalHeight / 2.f };

	for (sf::Text& text : texts)
	{
		auto bounds = text.getLocalBounds();

		text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y });

		text.setPosition({ LOGICAL_SIZE.x * 0.5f, y });

		y += bounds.size.y + spacing;
	}
}

void Game::CenterTextX(sf::Text& text)
{
	sf::FloatRect bounds{ text.getLocalBounds() };
	text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y });
	text.setPosition({ LOGICAL_SIZE.x * 0.5f, text.getPosition().y });
}

void Game::CenterText(sf::Text& text, float y)
{
	sf::FloatRect bounds = text.getLocalBounds();
	text.setOrigin({
		bounds.position.x + bounds.size.x * 0.5f,
		bounds.position.y
		});

	text.setPosition({ LOGICAL_SIZE.x * 0.5f, y });
}

sf::View Game::GetLetterboxView(const sf::View& view, int windowWidth, int windowHeight)
{
	if (windowWidth <= 0 || windowHeight <= 0)
		return view;

	float windowRatio{ static_cast<float>(windowWidth) / windowHeight };
	float viewRatio{ view.getSize().x / view.getSize().y };

	float sizeX{ 1.f };
	float sizeY{ 1.f };
	float posX{ 0.f };
	float posY{ 0.f };

	if (windowRatio > viewRatio)
	{
		sizeX = viewRatio / windowRatio;
		posX = (1.f - sizeX) / 2.f;
	}
	else
	{
		sizeY = windowRatio / viewRatio;
		posY = (1.f - sizeY) / 2.f;
	}

	sf::View newView{ view };
	newView.setViewport(sf::FloatRect({ posX, posY }, { sizeX, sizeY }));

	return newView;
}