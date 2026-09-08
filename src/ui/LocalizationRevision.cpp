#include "LocalizationRevision.h"

#include "localization/LocalizationManager.h"

namespace UI
{
	bool LocalizationRevision::Update(const LocalizationManager& localization) noexcept
	{
		const std::size_t current = localization.GetLanguageRevision();

		if (current == revision)
			return false;

		revision = current;

		return true;
	}
}