#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

#include <SFML/System/String.hpp>

#include "settings/GameSettings.h"
#include "utils/ConfigEnums.h"

class SettingsManager;

class LocalizationManager
{
public:
    explicit LocalizationManager(SettingsManager& settings);

    bool Load(const std::filesystem::path& directory);
    bool SetLanguage(Language language);

    [[nodiscard]] Language GetLanguage() const noexcept;
    [[nodiscard]] std::size_t GetRevision() const noexcept;
	[[nodiscard]] Config::Font RegularFont(bool keepEnglishDisplayStyle = true) const noexcept;
	[[nodiscard]] Config::Font BoldFont(bool keepEnglishDisplayStyle = true) const noexcept;
    [[nodiscard]] sf::String Get(std::string_view key) const;
	[[nodiscard]] sf::String Format(
		std::string_view key, std::string_view token, std::string_view value) const;

	// Concatenates every string in a language's catalog (Arabic-shaped the
	// same way Get() would) into one sample, for text-rendering warmup: a
	// single throwaway sf::Text built from this is guaranteed to touch every
	// character that language could ever actually display.
	[[nodiscard]] sf::String BuildWarmupText(Language language) const;

    [[nodiscard]] static std::string_view Code(Language language) noexcept;
    [[nodiscard]] static sf::String NativeName(Language language);

private:
    using Catalog = std::unordered_map<std::string, std::string>;

    [[nodiscard]] bool LoadCatalog(Language language, const std::filesystem::path& path);

    SettingsManager& settings;
    std::unordered_map<Language, Catalog> catalogs;
	std::size_t revision{ 0u };
};
