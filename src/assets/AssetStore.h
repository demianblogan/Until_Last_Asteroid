#pragma once

#include <unordered_map>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Window/Cursor.hpp>
#include "AssetStorage.h"
#include "utils/ConfigEnums.h"

class AssetStore
{
public:
	AssetStore() = default;

	AssetStore(const AssetStore&) = delete;
	AssetStore& operator=(const AssetStore&) = delete;

	AssetStore(AssetStore&&) = default;
	AssetStore& operator=(AssetStore&&) = default;

public:
	void Initialize();

	[[nodiscard]] AssetStorage<sf::Texture, Config::Texture>& Textures() noexcept;
	[[nodiscard]] const AssetStorage<sf::Texture, Config::Texture>& Textures() const noexcept;
	[[nodiscard]] AssetStorage<sf::Font, Config::Font>& Fonts() noexcept;
	[[nodiscard]] const AssetStorage<sf::Font, Config::Font>& Fonts() const noexcept;
	[[nodiscard]] AssetStorage<sf::SoundBuffer, Config::Sound>& Sounds() noexcept;
	[[nodiscard]] const AssetStorage<sf::SoundBuffer, Config::Sound>& Sounds() const noexcept;
	[[nodiscard]] AssetStorage<sf::Music, Config::Music>& Music() noexcept;
	[[nodiscard]] const AssetStorage<sf::Music, Config::Music>& Music() const noexcept;
	[[nodiscard]] sf::Shader& GetShader(Config::Shader id);
	[[nodiscard]] sf::Cursor& GetCursor(Config::Cursor id);

private:
	AssetStorage<sf::Texture, Config::Texture> textures;
	AssetStorage<sf::Font, Config::Font> fonts;
	AssetStorage<sf::SoundBuffer, Config::Sound> sounds;
	AssetStorage<sf::Music, Config::Music> music;
	std::unordered_map<Config::Shader, sf::Shader> shaders;

	// Cursors are not stored in AssetStorage because they require custom creation
	// (sf::Cursor::createFromPixels) instead of standard loadFromFile/openFromFile.
	std::unordered_map<Config::Cursor, sf::Cursor> cursors;

private:
	void InitializeTextures();
	void InitializeFonts();
	void InitializeSounds();
	void InitializeMusic();
	void InitializeShaders();
	void InitializeCursors();
};
