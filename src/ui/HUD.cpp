#include "HUD.h"

#include <string>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/AssetStore.h"
#include "game/GameplaySession.h"
#include "utils/ConfigEnums.h"

HUD::HUD(AssetStore& assets, const GameplaySession& session)
	: assets(assets)
	, session(session)
	, scoreText(assets.Fonts().Get(Config::Font::GUI))
	, lifeSprite(assets.Textures().Get(Config::Texture::PlayerLife))
{
	scoreText.setCharacterSize(SCORE_FONT_SIZE);
	scoreText.setPosition({ 20.f, 20.f });

	lifeSprite.setScale({ 2.f, 2.f });
}

void HUD::Update()
{
	scoreText.setString(std::to_string(session.GetScore()));
}

void HUD::Draw(sf::RenderTarget& target) const
{
	target.draw(scoreText);

	float offsetFromLeftBound{ 20 };
	for (int i{ 0 }; i < session.GetLives(); i++)
	{
		sf::Sprite spriteCopy = lifeSprite;
		spriteCopy.setPosition({ offsetFromLeftBound + LIFE_OFFSET.x * i, LIFE_OFFSET.y });

		target.draw(spriteCopy);
	}
}
