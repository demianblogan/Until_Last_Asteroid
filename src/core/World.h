#pragma once

#include <vector>
#include <memory>
#include <SFML/Audio/Sound.hpp>
#include <SFML/System/Vector2.hpp>

#include "Entity.h"
#include "utils/ConfigEnums.h"
#include "systems/InputHandler.h"

class AssetStore;
class GameState;
class Player;

namespace sf
{
	class RenderTarget;
	struct RenderStates;
	class RenderWindow;
	class Event;
	class Sound;
}

class Player;
class PlayerShot;
class SaucerShot;

class World : public sf::Drawable
{
public:
	World(unsigned int width, unsigned int height, AssetStore& assets, GameState& gameState);

	void Update(float deltaTime);

	void Spawn(std::unique_ptr<Entity> entity);
	void SpawnPlayerShot(const sf::Vector2f& pos, float rotation);
	void SpawnSaucerShot(const sf::Vector2f& pos, const sf::Vector2f& target);

	void AddSound(Config::Sound id);

	[[nodiscard]] sf::Vector2f GetPlayerPosition() const noexcept;
	[[nodiscard]] unsigned int GetWidth() const noexcept;
	[[nodiscard]] unsigned int GetHeight() const noexcept;

	sf::RenderWindow& GetWindow() noexcept;
	void SetWindow(sf::RenderWindow& window);

	void Clear();
	bool IsCleared() const noexcept;

	bool HasPlayer() const noexcept;
	void SpawnPlayer(AssetStore& assets, InputHandler<Config::PlayerAction>& input);

	void HandlePlayerEvent(const sf::Event& event);
	void HandlePlayerRealtime();

private:
	void Wrap(Entity& e) const;
	void HandleCollisions();
	void OnCollision(Entity& entity, const Entity& other);
	void RemoveDeadEntities();
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

	std::vector<std::unique_ptr<Entity>> entities;
	std::vector<std::unique_ptr<Entity>> pendingEntities;
	std::vector<std::unique_ptr<sf::Sound>> sounds;

	AssetStore& assets;
	GameState& gameState;

	Player* player{ nullptr };
	sf::RenderWindow* window{ nullptr };

	unsigned int width;
	unsigned int height;
};