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

	bool LoadCatalogs(const std::filesystem::path& directory);
	bool SetLanguage(Language language);

	[[nodiscard]] Language GetCurrentLanguage() const noexcept;
	[[nodiscard]] std::size_t GetLanguageRevision() const noexcept;
	[[nodiscard]] Config::Font GetRegularFont(bool keepEnglishDisplayStyle = true) const noexcept;
	[[nodiscard]] Config::Font GetBoldFont(bool keepEnglishDisplayStyle = true) const noexcept;
	[[nodiscard]] sf::String GetText(std::string_view key) const;
	[[nodiscard]] sf::String FormatText(std::string_view key, std::string_view token, std::string_view value) const;

	// Concatenates every string in a language's catalog into one sample, 
	// for text-rendering warmup: a single throwaway sf::Text built from
	// this is guaranteed to touch every character that language could
	// ever actually display.
	[[nodiscard]] sf::String BuildWarmupText(Language language) const;

	[[nodiscard]] static std::string_view GetLanguageCode(Language language) noexcept;
	[[nodiscard]] static sf::String GetLanguageNativeName(Language language);

private:
	// key: localization key (e.g. "options.title"); value: localized string in the target language.
	using Catalog = std::unordered_map<std::string, std::string>; 

	[[nodiscard]] bool LoadCatalog(Language language, const std::filesystem::path& path);

	SettingsManager& settings;
	std::unordered_map<Language, Catalog> catalogs;

	// Bumped whenever the active language changes, so callers can cheaply
	// detect a change instead of comparing strings.
	std::size_t languageRevision = 0u;
};
