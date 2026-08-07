#include "AssetStore.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <SFML/Graphics/Image.hpp>

void AssetStore::Initialize()
{
	InitializeTextures();
	InitializeFonts();
	InitializeSounds();
	InitializeMusic();
	InitializeCursors();
}

AssetStorage<sf::Texture, Config::Texture>& AssetStore::Textures() noexcept
{
	return textures;
}

const AssetStorage<sf::Texture, Config::Texture>& AssetStore::Textures() const noexcept
{
	return textures;
}

AssetStorage<sf::Font, Config::Font>& AssetStore::Fonts() noexcept
{
	return fonts;
}

const AssetStorage<sf::Font, Config::Font>& AssetStore::Fonts() const noexcept
{
	return fonts;
}

AssetStorage<sf::SoundBuffer, Config::Sound>& AssetStore::Sounds() noexcept
{
	return sounds;
}

const AssetStorage<sf::SoundBuffer, Config::Sound>& AssetStore::Sounds() const noexcept
{
	return sounds;
}

AssetStorage<sf::Music, Config::Music>& AssetStore::Music() noexcept
{
	return music;
}

const AssetStorage<sf::Music, Config::Music>& AssetStore::Music() const noexcept
{
	return music;
}

sf::Cursor& AssetStore::GetCursor(Config::Cursor id)
{
	auto iterator{ cursors.find(id) };
	if (iterator == cursors.end())
		throw std::runtime_error("Cursor not found");

	return iterator->second;
}

void AssetStore::InitializeTextures()
{
	textures.LoadFromFile(Config::Texture::CompanyLogo, "assets/other/alone_bull_company.png");

	textures.LoadFromFile(Config::Texture::PlayerShip, "assets/sprites/player/ship.png");
	textures.LoadFromFile(Config::Texture::PlayerLife, "assets/sprites/player/life.png");

	textures.LoadFromFile(Config::Texture::BigEnemySaucer, "assets/sprites/enemies/big_enemy_saucer.png");
	textures.LoadFromFile(Config::Texture::SmallEnemySaucer, "assets/sprites/enemies/small_enemy_saucer.png");

	textures.LoadFromFile(Config::Texture::BigMeteor1, "assets/sprites/meteors/big_meteor_1.png");
	textures.LoadFromFile(Config::Texture::BigMeteor2, "assets/sprites/meteors/big_meteor_2.png");
	textures.LoadFromFile(Config::Texture::BigMeteor3, "assets/sprites/meteors/big_meteor_3.png");
	textures.LoadFromFile(Config::Texture::BigMeteor4, "assets/sprites/meteors/big_meteor_4.png");

	textures.LoadFromFile(Config::Texture::MediumMeteor1, "assets/sprites/meteors/medium_meteor_1.png");
	textures.LoadFromFile(Config::Texture::MediumMeteor2, "assets/sprites/meteors/medium_meteor_2.png");

	textures.LoadFromFile(Config::Texture::SmallMeteor1, "assets/sprites/meteors/small_meteor_1.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor2, "assets/sprites/meteors/small_meteor_2.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor3, "assets/sprites/meteors/small_meteor_3.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor4, "assets/sprites/meteors/small_meteor_4.png");

	textures.LoadFromFile(Config::Texture::PlayerShot, "assets/sprites/shots/player_shot.png");
	textures.LoadFromFile(Config::Texture::EnemySaucerShot, "assets/sprites/shots/enemy_saucer_shot.png");
}

void AssetStore::InitializeFonts()
{
	fonts.LoadFromFile(Config::Font::GUI, "assets/fonts/trs_million.ttf");
}

void AssetStore::InitializeSounds()
{
	sounds.LoadFromFile(Config::Sound::PlayerLaserShot, "assets/audio/sounds/player_laser_shot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyLaserShot, "assets/audio/sounds/enemy_laser_shot.ogg");

	sounds.LoadFromFile(Config::Sound::SaucerKamikazeSpawn, "assets/audio/sounds/saucer_kamikaze_spawn.flac");
	sounds.LoadFromFile(Config::Sound::SaucerShooterSpawn, "assets/audio/sounds/saucer_shooter_spawn.flac");

	sounds.LoadFromFile(Config::Sound::PlayerShipExplosion, "assets/audio/sounds/player_ship_explosion.flac");
	sounds.LoadFromFile(Config::Sound::EnemySaucerExplosion, "assets/audio/sounds/enemy_saucer_explosion.flac");

	sounds.LoadFromFile(Config::Sound::BigMeteorExplosion, "assets/audio/sounds/big_meteor_explosion.flac");
	sounds.LoadFromFile(Config::Sound::MediumMeteorExplosion, "assets/audio/sounds/medium_meteor_explosion.flac");
	sounds.LoadFromFile(Config::Sound::SmallMeteorExplosion, "assets/audio/sounds/small_meteor_explosion.flac");
}

void AssetStore::InitializeMusic()
{
	music.LoadFromFile(Config::Music::CompanySplash, "assets/audio/music/company_splash.ogg");
	music.LoadFromFile(Config::Music::BackgroundTheme, "assets/audio/music/background_theme.ogg");
}

void AssetStore::InitializeCursors()
{
	auto arrowCursor{ sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow) };
	if (!arrowCursor.has_value())
		throw std::runtime_error("Failed to create system arrow cursor");

	cursors.emplace(Config::Cursor::Arrow, std::move(arrowCursor.value()));

	sf::Image cursorImage;
	std::string path{ "assets/sprites/crosshair.png" };

	if (!cursorImage.loadFromFile(path))
		throw std::runtime_error("Failed to load cursor image");

	const auto cursorImageSize{ cursorImage.getSize() };
	sf::Vector2u hotspot{ cursorImageSize.x / 2, cursorImageSize.y / 2 };

	auto cursorOpt{ sf::Cursor::createFromPixels(cursorImage.getPixelsPtr(), cursorImageSize, hotspot) };

	if (!cursorOpt.has_value())
		throw std::runtime_error("Failed to create cursor");

	cursors.emplace(Config::Cursor::Crosshair, std::move(cursorOpt.value()));
}
