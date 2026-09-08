#pragma once

#include <functional>
#include <optional>
#include <string_view>
#include <unordered_map>

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Audio/Music.hpp>
#include <SFML/Window/Cursor.hpp>

#include "AssetCache.h"
#include "gameplay/GameplayData.h"
#include "utils/ConfigEnums.h"

class Assets
{
public:
	using ProgressCallback = std::function<bool(float, std::string_view)>;

	Assets() = default;

	Assets(const Assets&) = delete;
	Assets& operator=(const Assets&) = delete;

	Assets(Assets&&) = default;
	Assets& operator=(Assets&&) = default;

public:
	[[nodiscard]] bool Initialize(const ProgressCallback& progress = {});

	[[nodiscard]] AssetCache<sf::Texture, Config::Texture>& Textures() noexcept;
	[[nodiscard]] const AssetCache<sf::Texture, Config::Texture>& Textures() const noexcept;

	[[nodiscard]] AssetCache<sf::Font, Config::Font>& Fonts() noexcept;
	[[nodiscard]] const AssetCache<sf::Font, Config::Font>& Fonts() const noexcept;

	[[nodiscard]] AssetCache<sf::SoundBuffer, Config::Sound>& Sounds() noexcept;
	[[nodiscard]] const AssetCache<sf::SoundBuffer, Config::Sound>& Sounds() const noexcept;

	[[nodiscard]] AssetCache<sf::Music, Config::Music>& Music() noexcept;
	[[nodiscard]] const AssetCache<sf::Music, Config::Music>& Music() const noexcept;

	[[nodiscard]] sf::Shader& GetShader(Config::Shader id);
	[[nodiscard]] sf::Cursor& GetCursor(Config::Cursor id);
	[[nodiscard]] const GameplayData& GetGameplayData() const;

private:
	AssetCache<sf::Texture, Config::Texture> textures;
	AssetCache<sf::Font, Config::Font> fonts;
	AssetCache<sf::SoundBuffer, Config::Sound> sounds;
	AssetCache<sf::Music, Config::Music> music;
	std::unordered_map<Config::Shader, sf::Shader> shaders;
	std::optional<GameplayData> gameplayData;

	// Cursors are not stored in AssetCache because they require custom creation
	// (sf::Cursor::createFromPixels) instead of standard loadFromFile/openFromFile.
	std::unordered_map<Config::Cursor, sf::Cursor> cursors;

private:
	void InitializeTextures();
	void InitializeFonts();
	void InitializeSounds();
	void InitializeMusic();
	void InitializeShaders();
	void InitializeCursors();
	void InitializeGameplayData();
};
