#include "World.h"

#include <algorithm>
#include <SFML/Graphics/RenderWindow.hpp>
#include "assets/AssetStore.h"
#include "entities/Enemy.h"
#include "GameState.h"
#include "entities/Player.h"
#include "entities/Shot.h"

// --------------------------------------------------------
World::World(unsigned int width, unsigned int height, AssetStore& assets, GameState& gameState)
	: width(width), height(height), assets(assets), gameState(gameState)
{
	// No code
}

// --------------------------------------------------------
void World::Update(float deltaTime)
{
	if (!pendingEntities.empty())
	{
		entities.insert(
			entities.end(),	
			std::make_move_iterator(pendingEntities.begin()),
			std::make_move_iterator(pendingEntities.end())
		);

		pendingEntities.clear();
	}

	for (auto& entity : entities)
	{
		entity->Update(deltaTime);

		if (entity->GetType() != Entity::Type::Projectile_Player &&
			entity->GetType() != Entity::Type::Projectile_Enemy)
		{
			Wrap(*entity);
		}
	}

	HandleCollisions();
	RemoveDeadEntities();

	sounds.erase(
		std::remove_if(sounds.begin(), sounds.end(),
			[](const std::unique_ptr<sf::Sound>& sound)
			{
				return sound->getStatus() != sf::Sound::Status::Playing;
			}),
		sounds.end()
	);
}

void World::Spawn(std::unique_ptr<Entity> entity)
{
	pendingEntities.push_back(std::move(entity));
}

void World::SpawnPlayer(AssetStore& assets, InputHandler<Config::PlayerAction>& input)
{
	if (player != nullptr)
		return;

	auto playerPtr{ std::make_unique<Player>(assets, *this, input) };
	playerPtr->SetPosition({ GetWidth() * 0.5f,	GetHeight() * 0.5f });

	player = playerPtr.get();
	Spawn(std::move(playerPtr));
}

sf::RenderWindow& World::GetWindow() noexcept
{
	return *window;
}

void World::SetWindow(sf::RenderWindow& window)
{
	this->window = &window;
}

bool World::IsCleared() const noexcept
{
	for (const auto& entity : entities)
	{
		if (!entity->IsAlive())
			continue;

		if (entity->GetType() == Entity::Type::Enemy ||
			entity->GetType() == Entity::Type::Asteroid)
		{
			return false;
		}
	}

	return true;
}

bool World::HasPlayer() const noexcept
{
	return player != nullptr;
}

void World::HandlePlayerEvent(const sf::Event& event)
{
	if (player != nullptr)
		player->HandleEvent(event);
}

void World::HandlePlayerRealtime()
{
	if (player != nullptr)
		player->HandleRealtime();
}

void World::SpawnPlayerShot(const sf::Vector2f& pos, float rotation)
{
	Spawn(std::make_unique<PlayerShot>(assets, *this, pos, rotation));
}

void World::SpawnSaucerShot(const sf::Vector2f& pos,
	const sf::Vector2f& target)
{
	Spawn(std::make_unique<SaucerShot>(assets, *this, pos, target, gameState.GetScore()));
}

void World::AddSound(Config::Sound id)
{
	auto sound = std::make_unique<sf::Sound>(assets.Sounds().Get(id));
	sound->setAttenuation(0.f);
	sound->play();

	sounds.push_back(std::move(sound));
}

sf::Vector2f World::GetPlayerPosition() const noexcept
{
	return player != nullptr ? player->GetPosition() : sf::Vector2f{};
}

unsigned int World::GetWidth() const noexcept
{
	return width;
}

unsigned int World::GetHeight() const noexcept
{
	return height;
}

void World::Clear()
{
	entities.clear();
	pendingEntities.clear();
	sounds.clear();

	player = nullptr;
}

void World::Wrap(Entity& e) const
{
	auto position = e.GetPosition();

	if (position.x < 0)
		position.x = static_cast<float>(width);
	else if (position.x > width)
		position.x = 0.f;

	if (position.y < 0)
		position.y = static_cast<float>(height);
	else if (position.y > height)
		position.y = 0.f;

	e.SetPosition(position);
}

void World::HandleCollisions()
{
	for (size_t i{ 0 }; i < entities.size(); i++)
	{
		for (size_t j = { i + 1 }; j < entities.size(); j++)
		{
			Entity& a = *entities[i];
			Entity& b = *entities[j];

			if (!a.IsAlive() || !b.IsAlive())
				continue;

			if (a.IsCollideWith(b) && b.IsCollideWith(a))
			{
				OnCollision(a);
				OnCollision(b);
			}
		}
	}
}

void World::OnCollision(Entity& entity)
{
	if (entity.GetType() == Entity::Type::Player)
	{
		gameState.LoseLife();

		if (gameState.GetLives() <= 0)
			gameState.SetGameOver();
	}

	entity.Destroy();

	if (entity.GetType() == Entity::Type::Enemy ||
		entity.GetType() == Entity::Type::Asteroid)
	{
		Enemy* enemy = dynamic_cast<Enemy*>(&entity);
		if (enemy != nullptr)
		{
			gameState.AddScore(enemy->GetScoreValue());
		}
	}
}

void World::RemoveDeadEntities()
{
	for (size_t i{ 0 }; i < entities.size();)
	{
		if (!entities[i]->IsAlive())
		{
			if (entities[i].get() == player)
				player = nullptr;

			entities[i] = std::move(entities.back());
			entities.pop_back();
		}
		else
		{
			i++;
		}
	}
}

void World::draw(sf::RenderTarget& target,
	sf::RenderStates states) const
{
	for (const auto& entity : entities)
		target.draw(*entity, states);
}