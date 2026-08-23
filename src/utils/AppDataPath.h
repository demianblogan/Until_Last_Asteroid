#pragma once

#include <filesystem>
#include <string_view>

// Shared per-player save/config file location logic, used by every manager
// that persists something to disk (achievements, campaign progress,
// records, settings).
namespace AppDataPath
{
	// Resolves the full path to a per-player file named fileName. Prefers
	// %LOCALAPPDATA%\Alone Bull Company\Until Last Asteroid\<fileName> (the
	// standard per-user, non-roaming location Windows recommends for this
	// kind of data); falls back to a user_data\ folder next to the
	// executable if LOCALAPPDATA isn't available for some reason.
	[[nodiscard]] std::filesystem::path Resolve(std::string_view fileName);
}