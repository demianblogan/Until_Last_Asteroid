#pragma once

#include <cstddef>

class LocalizationManager;

namespace UI
{
	// Tracks the localization language revision so callers can lazily refresh
	// their localized content only when the language actually changed, instead
	// of re-applying it every frame.
	class LocalizationRevision
	{
	public:
		// Returns true (and records the new revision) exactly when the language
		// has changed since the last call. The return value is intentionally
		// discardable: constructors use this to prime the tracker to the
		// current revision without caring whether that counts as "changed".
		bool Update(const LocalizationManager& localization) noexcept;

	private:
		std::size_t revision = 0u;
	};
}