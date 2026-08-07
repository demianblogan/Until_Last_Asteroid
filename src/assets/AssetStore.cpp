#include "AssetStore.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <SFML/Graphics/Image.hpp>

namespace
{
	sf::Cursor LoadCursor(const std::string& path, sf::Vector2u hotspot)
	{
		sf::Image image;
		if (!image.loadFromFile(path))
			throw std::runtime_error("Failed to load cursor image: " + path);

		auto cursor{ sf::Cursor::createFromPixels(image.getPixelsPtr(), image.getSize(), hotspot) };
		if (!cursor.has_value())
			throw std::runtime_error("Failed to create cursor: " + path);

		return std::move(cursor.value());
	}
}

void AssetStore::Initialize()
{
	InitializeTextures();
	InitializeFonts();
	InitializeSounds();
	InitializeMusic();
	InitializeShaders();
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

sf::Shader& AssetStore::GetShader(Config::Shader id)
{
	auto iterator{ shaders.find(id) };
	if (iterator == shaders.end())
		throw std::runtime_error("Shader not found");

	return iterator->second;
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
	textures.LoadFromFile(Config::Texture::MainMenuBackground, "assets/backgrounds/main_menu_background.png");
	textures.LoadFromFile(Config::Texture::MenuButtonIdle, "assets/sprites/ui/menu_button_idle.png");
	textures.LoadFromFile(Config::Texture::MenuButtonSelected, "assets/sprites/ui/menu_button_selected.png");

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
	fonts.LoadFromFile(Config::Font::MenuRegular, "assets/fonts/orbitron_regular.ttf");
	fonts.LoadFromFile(Config::Font::MenuSemibold, "assets/fonts/orbitron_semibold.ttf");
}

void AssetStore::InitializeSounds()
{
	sounds.LoadFromFile(Config::Sound::CharacterTyping, "assets/audio/sounds/character_typing.ogg");
	sounds.LoadFromFile(Config::Sound::InterfaceActivation, "assets/audio/sounds/interface_activation.ogg");
	sounds.LoadFromFile(Config::Sound::ItemSelect, "assets/audio/sounds/item_select.ogg");
	sounds.LoadFromFile(Config::Sound::ItemPress, "assets/audio/sounds/item_press.ogg");

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
	music.LoadFromFile(Config::Music::MainMenuBackground, "assets/audio/music/main_menu_background.ogg");
	music.LoadFromFile(Config::Music::GameplayTheme, "assets/audio/music/gameplay_theme.ogg");
}

void AssetStore::InitializeShaders()
{
	sf::Shader blurShader;
	const std::string blurPath{ "assets/shaders/gaussian_blur.frag" };
	if (!blurShader.loadFromFile(blurPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + blurPath);

	shaders.emplace(Config::Shader::GaussianBlur, std::move(blurShader));

	sf::Shader brightPassShader;
	const std::string brightPassPath{ "assets/shaders/bright_pass.frag" };
	if (!brightPassShader.loadFromFile(brightPassPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + brightPassPath);

	shaders.emplace(Config::Shader::BrightPass, std::move(brightPassShader));
}

void AssetStore::InitializeCursors()
{
	cursors.emplace(
		Config::Cursor::MenuPointer,
		LoadCursor("assets/cursors/menu_pointer.png", { 6u, 2u }));

	cursors.emplace(
		Config::Cursor::GameplayCrosshair,
		LoadCursor("assets/cursors/gameplay_crosshair.png", { 32u, 32u }));
}
