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
	textures.LoadFromFile(Config::Texture::PlayerShip, "assets/sprites/player/Ship.png");
	textures.LoadFromFile(Config::Texture::PlayerLife, "assets/sprites/player/Life.png");

	textures.LoadFromFile(Config::Texture::BigEnemySaucer, "assets/sprites/enemies/BigEnemySaucer.png");
	textures.LoadFromFile(Config::Texture::SmallEnemySaucer, "assets/sprites/enemies/SmallEnemySaucer.png");

	textures.LoadFromFile(Config::Texture::BigMeteor1, "assets/sprites/meteors/BigMeteor1.png");
	textures.LoadFromFile(Config::Texture::BigMeteor2, "assets/sprites/meteors/BigMeteor2.png");
	textures.LoadFromFile(Config::Texture::BigMeteor3, "assets/sprites/meteors/BigMeteor3.png");
	textures.LoadFromFile(Config::Texture::BigMeteor4, "assets/sprites/meteors/BigMeteor4.png");

	textures.LoadFromFile(Config::Texture::MediumMeteor1, "assets/sprites/meteors/MediumMeteor1.png");
	textures.LoadFromFile(Config::Texture::MediumMeteor2, "assets/sprites/meteors/MediumMeteor2.png");

	textures.LoadFromFile(Config::Texture::SmallMeteor1, "assets/sprites/meteors/SmallMeteor1.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor2, "assets/sprites/meteors/SmallMeteor2.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor3, "assets/sprites/meteors/SmallMeteor3.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor4, "assets/sprites/meteors/SmallMeteor4.png");

	textures.LoadFromFile(Config::Texture::PlayerShot, "assets/sprites/Shots/PlayerShot.png");
	textures.LoadFromFile(Config::Texture::EnemySaucerShot, "assets/sprites/Shots/EnemySaucerShot.png");
}

void AssetStore::InitializeFonts()
{
	fonts.LoadFromFile(Config::Font::GUI, "assets/fonts/trs-million.ttf");
}

void AssetStore::InitializeSounds()
{
	sounds.LoadFromFile(Config::Sound::PlayerLaserShot, "assets/sounds/PlayerLaserShot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyLaserShot, "assets/sounds/EnemyLaserShot.ogg");

	sounds.LoadFromFile(Config::Sound::SaucerKamikazeSpawn, "assets/sounds/SaucerKamikazeSpawn.flac");
	sounds.LoadFromFile(Config::Sound::SaucerShooterSpawn, "assets/sounds/SaucerShooterSpawn.flac");

	sounds.LoadFromFile(Config::Sound::PlayerShipExplosion, "assets/sounds/PlayerShipExplosion.flac");
	sounds.LoadFromFile(Config::Sound::EnemySaucerExplosion, "assets/sounds/EnemySaucerExplosion.flac");

	sounds.LoadFromFile(Config::Sound::BigMeteorExplosion, "assets/sounds/BigMeteorExplosion.flac");
	sounds.LoadFromFile(Config::Sound::MediumMeteorExplosion, "assets/sounds/MediumMeteorExplosion.flac");
	sounds.LoadFromFile(Config::Sound::SmallMeteorExplosion, "assets/sounds/SmallMeteorExplosion.flac");
}

void AssetStore::InitializeMusic()
{
	music.LoadFromFile(Config::Music::BackgroundTheme, "assets/music/BackgroundTheme.ogg");
}

void AssetStore::InitializeCursors()
{
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