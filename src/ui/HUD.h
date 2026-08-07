#pragma once

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

class AssetStore;
class GameplaySession;

namespace sf
{
	class RenderTarget;
}

class HUD
{
public:
	HUD(AssetStore& assets, const GameplaySession& session);

	void Update();
	void Draw(sf::RenderTarget& target) const;

private:
	AssetStore& assets;
	const GameplaySession& session;

	sf::Text scoreText;
	sf::Sprite lifeSprite;

	static constexpr int SCORE_FONT_SIZE = 50;
	static constexpr sf::Vector2f LIFE_OFFSET{ 80.f, 90.0f };
};
