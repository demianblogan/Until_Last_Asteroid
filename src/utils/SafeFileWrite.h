#pragma once

#include <filesystem>
#include <system_error>

// Shared crash-safe file writing helpers used by every persisted save/config
// file in the game (achievements, campaign progress, records, settings).
// Write new content to a temp file first, then call ReplaceFileAtomically to
// swap it into place -- a crash, power loss, or antivirus lock mid-write can
// then never leave the player with a missing or half-written file; at worst
// they keep the old (still valid) file from before this save.
namespace SafeFileWrite
{
	// Named check so every filesystem-error test at call sites reads as a
	// plain sentence ("if HasFailed(error)") instead of relying on
	// std::error_code's operator bool.
	[[nodiscard]] bool HasFailed(const std::error_code& error) noexcept;

	// Swaps a freshly written temp file into targetPath's place. Backs the
	// previous file up to <targetPath>.bak first and rolls back to it if the
	// final swap fails, so a mid-operation failure never leaves targetPath
	// missing.
	[[nodiscard]] bool ReplaceFileAtomically(
		const std::filesystem::path& temporaryPath, const std::filesystem::path& targetPath);

	// Renames a file that failed to load/parse to <path>.corrupt (or
	// .corrupt.1, .corrupt.2, ... if that name is already taken), instead of
	// deleting or silently overwriting it -- so a corrupted save isn't lost
	// forever, it just stays on disk under a different name for later
	// inspection while the caller starts fresh.
	[[nodiscard]] bool PreserveCorruptFile(const std::filesystem::path& path);
}