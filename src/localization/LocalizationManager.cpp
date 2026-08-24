#include "LocalizationManager.h"

#include <array>
#include <algorithm>
#include <fstream>
#include <optional>

#include <nlohmann/json.hpp>

#include "settings/SettingsManager.h"

namespace
{
	using Json = nlohmann::json;

	constexpr std::array Languages =
	{
		Language::English,
		Language::Spanish,
		Language::Russian,
		Language::Ukrainian,
		Language::Arabic
	};

	// Recursively turns a nested JSON object into a flat "dotted.key" -> value map,
	// e.g. {"options": {"title": "..."}} becomes {"options.title": "..."}.
	// keyPrefix is the dotted key accumulated so far (empty on the initial call);
	// output is filled in place since the recursion shares one map across all calls.
	void FlattenJsonCatalog(const Json& value, const std::string& keyPrefix,
		std::unordered_map<std::string, std::string>& output)
	{
		for (const auto& [key, child] : value.items())
		{
			const std::string fullKey = keyPrefix.empty() ? key : keyPrefix + "." + key;

			if (child.is_object())
				FlattenJsonCatalog(child, fullKey, output);

			else if (child.is_string())
				output.emplace(fullKey, child.get<std::string>());
		}
	}

	sf::String FromUtf8(std::string_view value)
	{
		return sf::String::fromUtf8(value.begin(), value.end());
	}

	// Arabic letters are written cursively and change shape depending on their
	// neighbors, the same way handwritten cursive Latin letters join up: a letter's
	// glyph differs depending on whether it stands alone or connects to the
	// letter(s) before and/or after it in the word.
	struct ArabicForms
	{
		char32_t base;      // The letter's plain Unicode code point, as it appears unshaped
		// (e.g. in JSON source text).
		char32_t isolated;  // Glyph used when the letter joins neither neighbor (standing alone).
		char32_t final;     // Glyph used when the letter joins only the previous letter (word-final position).
		char32_t initial;   // Glyph used when the letter joins only the next letter (word-initial position).
		char32_t medial;    // Glyph used when the letter joins both neighbors (word-middle position). 
		// 0 if this letter never joins on its left side, so it has no medial/initial form.
	};

	// One entry per Arabic letter, giving its four positional glyph variants.
	// The `base` code points are the standard Arabic block (U+0600-U+06FF); the
	// shaped variants are their presentation-form code points from the Unicode
	// "Arabic Presentation Forms-B" block (U+FE70-U+FEFF), which is exactly the
	// block designed to hold pre-shaped Arabic glyphs. Both are official Unicode
	// standard values, not project-specific choices.
	constexpr std::array ArabicLetters =
	{
		ArabicForms{ U'ء', 0xFE80, 0, 0, 0 },
		ArabicForms{ U'آ', 0xFE81, 0xFE82, 0, 0 },
		ArabicForms{ U'أ', 0xFE83, 0xFE84, 0, 0 },
		ArabicForms{ U'ؤ', 0xFE85, 0xFE86, 0, 0 },
		ArabicForms{ U'إ', 0xFE87, 0xFE88, 0, 0 },
		ArabicForms{ U'ئ', 0xFE89, 0xFE8A, 0xFE8B, 0xFE8C },
		ArabicForms{ U'ا', 0xFE8D, 0xFE8E, 0, 0 },
		ArabicForms{ U'ب', 0xFE8F, 0xFE90, 0xFE91, 0xFE92 },
		ArabicForms{ U'ة', 0xFE93, 0xFE94, 0, 0 },
		ArabicForms{ U'ت', 0xFE95, 0xFE96, 0xFE97, 0xFE98 },
		ArabicForms{ U'ث', 0xFE99, 0xFE9A, 0xFE9B, 0xFE9C },
		ArabicForms{ U'ج', 0xFE9D, 0xFE9E, 0xFE9F, 0xFEA0 },
		ArabicForms{ U'ح', 0xFEA1, 0xFEA2, 0xFEA3, 0xFEA4 },
		ArabicForms{ U'خ', 0xFEA5, 0xFEA6, 0xFEA7, 0xFEA8 },
		ArabicForms{ U'د', 0xFEA9, 0xFEAA, 0, 0 },
		ArabicForms{ U'ذ', 0xFEAB, 0xFEAC, 0, 0 },
		ArabicForms{ U'ر', 0xFEAD, 0xFEAE, 0, 0 },
		ArabicForms{ U'ز', 0xFEAF, 0xFEB0, 0, 0 },
		ArabicForms{ U'س', 0xFEB1, 0xFEB2, 0xFEB3, 0xFEB4 },
		ArabicForms{ U'ش', 0xFEB5, 0xFEB6, 0xFEB7, 0xFEB8 },
		ArabicForms{ U'ص', 0xFEB9, 0xFEBA, 0xFEBB, 0xFEBC },
		ArabicForms{ U'ض', 0xFEBD, 0xFEBE, 0xFEBF, 0xFEC0 },
		ArabicForms{ U'ط', 0xFEC1, 0xFEC2, 0xFEC3, 0xFEC4 },
		ArabicForms{ U'ظ', 0xFEC5, 0xFEC6, 0xFEC7, 0xFEC8 },
		ArabicForms{ U'ع', 0xFEC9, 0xFECA, 0xFECB, 0xFECC },
		ArabicForms{ U'غ', 0xFECD, 0xFECE, 0xFECF, 0xFED0 },
		ArabicForms{ U'ف', 0xFED1, 0xFED2, 0xFED3, 0xFED4 },
		ArabicForms{ U'ق', 0xFED5, 0xFED6, 0xFED7, 0xFED8 },
		ArabicForms{ U'ك', 0xFED9, 0xFEDA, 0xFEDB, 0xFEDC },
		ArabicForms{ U'ل', 0xFEDD, 0xFEDE, 0xFEDF, 0xFEE0 },
		ArabicForms{ U'م', 0xFEE1, 0xFEE2, 0xFEE3, 0xFEE4 },
		ArabicForms{ U'ن', 0xFEE5, 0xFEE6, 0xFEE7, 0xFEE8 },
		ArabicForms{ U'ه', 0xFEE9, 0xFEEA, 0xFEEB, 0xFEEC },
		ArabicForms{ U'و', 0xFEED, 0xFEEE, 0, 0 },
		ArabicForms{ U'ى', 0xFEEF, 0xFEF0, 0, 0 },
		ArabicForms{ U'ي', 0xFEF1, 0xFEF2, 0xFEF3, 0xFEF4 }
	};

	const ArabicForms* FindArabic(char32_t character)
	{
		const auto found = std::ranges::find(ArabicLetters, character, &ArabicForms::base);
		return found == ArabicLetters.end() ? nullptr : &*found;
	}

	// Replaces each Arabic letter in a logical-order line with its positional glyph
	// form (isolated/initial/medial/final -- see ArabicForms above), based on
	// whether its neighbors are also Arabic letters that can join with it.
	std::u32string ApplyArabicGlyphForms(const std::u32string& logicalLine)
	{
		std::u32string shaped = logicalLine;
		for (std::size_t index = 0; index < shaped.size(); index++)
		{
			const ArabicForms* current = FindArabic(logicalLine[index]);
			if (!current)
				continue;

			const ArabicForms* previous = index > 0u ? FindArabic(logicalLine[index - 1u]) : nullptr;
			const ArabicForms* next = index + 1u < logicalLine.size() ? FindArabic(logicalLine[index + 1u]) : nullptr;

			const bool isJoiningPrevious = previous && previous->initial && current->final;
			const bool isJoiningNext = next && current->initial && next->final;

			shaped[index] = isJoiningPrevious && isJoiningNext ? current->medial
				: isJoiningPrevious ? current->final
				: isJoiningNext ? current->initial : current->isolated;
		}
		return shaped;
	}

	// Reversing an RTL line for display also reverses URLs, e-mail addresses and
	// numbers, which are still meant to read left-to-right. Restores every
	// contiguous ASCII run back to its original internal order, while leaving it
	// in its (now reversed) position within the line.
	constexpr char32_t FirstNonASCIICodePoint = 128u;

	void RestoreASCIIRunOrder(std::u32string& line)
	{
		for (std::size_t runStart = 0; runStart < line.size();)
		{
			if (line[runStart] >= FirstNonASCIICodePoint || line[runStart] == U' ')
			{
				runStart++;
				continue;
			}

			std::size_t runEnd = runStart + 1u;
			while (runEnd < line.size() && line[runEnd] < FirstNonASCIICodePoint && line[runEnd] != U' ')
				runEnd++;

			std::reverse(line.begin() + static_cast<std::ptrdiff_t>(runStart),
				line.begin() + static_cast<std::ptrdiff_t>(runEnd));

			runStart = runEnd;
		}
	}

	// "Shaping" is the typographic term for turning logical-order text (characters
	// in reading order, as stored in the JSON catalog) into the actual glyphs and
	// order needed to display it correctly -- here, picking each Arabic letter's
	// positional form and reversing each line for RTL display.
	sf::String ReshapeArabicTextForDisplay(const sf::String& input)
	{
		std::u32string text = input.toUtf32();

		std::u32string output;
		output.reserve(text.size());

		std::size_t lineStart = 0;
		while (lineStart <= text.size())
		{
			const std::size_t lineEnd = text.find(U'\n', lineStart);
			const std::size_t end = lineEnd == std::u32string::npos ? text.size() : lineEnd;

			const std::u32string logicalLine(text.begin() + static_cast<std::ptrdiff_t>(lineStart),
				text.begin() + static_cast<std::ptrdiff_t>(end));

			std::u32string shapedLine = ApplyArabicGlyphForms(logicalLine);
			std::ranges::reverse(shapedLine);
			RestoreASCIIRunOrder(shapedLine);

			output.append(shapedLine);

			if (lineEnd == std::u32string::npos)
				break;

			output.push_back(U'\n');
			lineStart = lineEnd + 1u;
		}

		return sf::String::fromUtf32(output.begin(), output.end());
	}
}

LocalizationManager::LocalizationManager(SettingsManager& settingsManager)
	: settings(settingsManager)
{}

bool LocalizationManager::LoadCatalogs(const std::filesystem::path& directory)
{
	catalogs.clear();
	for (const Language language : Languages)
		if (!LoadCatalog(language, directory / (std::string(GetLanguageCode(language)) + ".json")))
			return false;

	const Catalog& english = catalogs.at(Language::English);

	for (const Language language : Languages)
	{
		const Catalog& catalog = catalogs.at(language);

		if (catalog.size() != english.size())
			return false;

		for (const auto& [key, _] : english)
			if (!catalog.contains(key))
				return false;
	}

	return true;
}

bool LocalizationManager::SetLanguage(Language language)
{
	if (!catalogs.contains(language))
		return false;

	const bool isLanguageChanged = settings.GetSettings().localization.language != language;

	settings.EditSettings().localization.language = language;
	settings.EditSettings().localization.isLanguageChosen = true;

	if (!settings.SaveSettings())
		return false;

	if (isLanguageChanged)
		languageRevision++;

	return true;
}

Language LocalizationManager::GetCurrentLanguage() const noexcept
{
	return settings.GetSettings().localization.language;
}

std::size_t LocalizationManager::GetLanguageRevision() const noexcept
{
	return languageRevision;
}

Config::Font LocalizationManager::GetRegularFont(bool keepEnglishDisplayStyle) const noexcept
{
	if (GetCurrentLanguage() == Language::Arabic)
		return Config::Font::ArabicRegular;
	else if (GetCurrentLanguage() == Language::English && keepEnglishDisplayStyle)
		return Config::Font::MenuRegular;
	else
		return Config::Font::LocalizedRegular;
}

Config::Font LocalizationManager::GetBoldFont(bool keepEnglishDisplayStyle) const noexcept
{
	if (GetCurrentLanguage() == Language::Arabic)
		return Config::Font::ArabicBold;
	else if (GetCurrentLanguage() == Language::English && keepEnglishDisplayStyle)
		return Config::Font::MenuSemibold;
	else
		return Config::Font::LocalizedBold;
}

sf::String LocalizationManager::GetText(std::string_view key) const
{
	const auto findIn = [key](const Catalog& catalog) -> const std::string*
		{
			const auto found{ catalog.find(std::string(key)) };
			return found == catalog.end() ? nullptr : &found->second;
		};

	if (const auto selected = catalogs.find(GetCurrentLanguage()); selected != catalogs.end())
	{
		if (const std::string * value{ findIn(selected->second) })
		{
			const sf::String result{ FromUtf8(*value) };
			return GetCurrentLanguage() == Language::Arabic ? ReshapeArabicTextForDisplay(result) : result;
		}
	}

	if (const auto english = catalogs.find(Language::English); english != catalogs.end())
		if (const std::string* value = findIn(english->second)) return FromUtf8(*value);

	return FromUtf8(key);
}

sf::String LocalizationManager::FormatText(std::string_view key, std::string_view token, std::string_view value) const
{
	const auto findRaw = [key](const Catalog& catalog) -> std::optional<std::string>
		{
			const auto found{ catalog.find(std::string(key)) };
			return found == catalog.end() ? std::nullopt : std::optional{ found->second };
		};

	std::optional<std::string> text;
	if (const auto selected = catalogs.find(GetCurrentLanguage()); selected != catalogs.end())
		text = findRaw(selected->second);

	if (!text)
		if (const auto english = catalogs.find(Language::English); english != catalogs.end())
			text = findRaw(english->second);

	if (!text)
		return GetText(key);

	const std::string marker = "{" + std::string(token) + "}";
	if (const std::size_t position = text->find(marker); position != std::string::npos)
		text->replace(position, marker.size(), value);

	const sf::String result = FromUtf8(*text);

	return GetCurrentLanguage() == Language::Arabic ? ReshapeArabicTextForDisplay(result) : result;
}

sf::String LocalizationManager::BuildWarmupText(Language language) const
{
	const auto found{ catalogs.find(language) };
	if (found == catalogs.end())
		return {};

	// A rough estimate of a full catalog's concatenated size, just to avoid a few
	// reallocations while joining -- not a hard limit, joined can grow past this.
	constexpr std::size_t EstimatedCatalogTextLength = 4096u;

	std::string joined;
	joined.reserve(EstimatedCatalogTextLength);

	for (const auto& [key, value] : found->second)
	{
		joined += value;
		joined += ' ';
	}

	const sf::String result = FromUtf8(joined);

	return language == Language::Arabic ? ReshapeArabicTextForDisplay(result) : result;
}

std::string_view LocalizationManager::GetLanguageCode(Language language) noexcept
{
	switch (language)
	{
	case Language::English:
		return "en";
	case Language::Spanish:
		return "es";
	case Language::Russian:
		return "ru";
	case Language::Ukrainian:
		return "uk";
	case Language::Arabic:
		return "ar";
	default:
		return "en";
	}
}

sf::String LocalizationManager::GetLanguageNativeName(Language language)
{
	switch (language)
	{
	case Language::English:
		return FromUtf8("English");
	case Language::Spanish:
		return FromUtf8("Español");
	case Language::Russian:
		return FromUtf8("Русский");
	case Language::Ukrainian:
		return FromUtf8("Українська");

		// SFML renders glyphs but does not perform Arabic shaping or bidi layout.
		// This native-name label therefore uses visual-order presentation forms;
		// general Arabic shaping is handled by the localized text layout layer.
	case Language::Arabic:
		return FromUtf8("ﺔﻴﺑﺮﻌﻟﺍ");

	default:
		return FromUtf8("English");
	}
}

bool LocalizationManager::LoadCatalog(Language language, const std::filesystem::path& path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return false;

	try
	{
		const Json root = Json::parse(file);
		if (!root.is_object())
			return false;

		Catalog catalog;
		FlattenJsonCatalog(root, "", catalog);

		return !catalog.empty() && catalogs.emplace(language, std::move(catalog)).second;
	}
	catch (const Json::exception&)
	{
		return false;
	}
}