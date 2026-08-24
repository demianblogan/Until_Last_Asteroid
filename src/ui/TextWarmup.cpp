#include "TextWarmup.h"

#include <array>
#include <span>

#include <SFML/Graphics/Text.hpp>

#include "assets/Assets.h"
#include "localization/LocalizationManager.h"
#include "settings/GameSettings.h"
#include "utils/ConfigEnums.h"

namespace TextWarmup
{
	namespace
	{
		// Every distinct character size passed to a Text/MenuButton
		// constructor anywhere in the game, split by which font "role"
		// (heading/bold vs. body/button/regular) actually uses it. Audited
		// against every call site in src/ -- keep this in sync whenever a
		// new size is introduced, since a missed one just moves its
		// first-touch glyph-rasterization hitch from here to whichever
		// screen uses it first.
		constexpr std::array BoldSizes{
			18u, 20u, 22u, 27u, 29u, 30u, 31u, 34u, 36u, 38u, 42u, 60u, 68u, 72u, 76u, 82u, 86u, 92u, 104u };
		constexpr std::array RegularSizes{
			14u, 20u, 21u, 23u, 24u, 25u, 26u, 27u, 28u, 29u, 30u, 31u, 34u, 36u, 38u, 48u };
		constexpr std::array BodyRegularSizes{ 27u, 29u };

		// Total number of (font, size) combinations Run() touches, regardless
		// of which sample texts turn out non-empty -- used as the denominator
		// for progress reporting.
		constexpr std::size_t TotalCombinationCount{
			BoldSizes.size() + RegularSizes.size() + BodyRegularSizes.size() +
			BoldSizes.size() + RegularSizes.size() +
			BoldSizes.size() + RegularSizes.size() };

		// Returns false if the callback requested cancellation.
		[[nodiscard]] bool WarmupFont(
			sf::Font& font, const sf::String& sampleText, std::span<const unsigned int> sizes,
			const TextWarmup::ProgressCallback& progress,
			std::size_t& combinationsTouched)
		{
			if (sampleText.isEmpty())
				return true;

			for (const unsigned int size : sizes)
			{
				sf::Text text(font, sampleText, size);
				// The bounds themselves are unused -- calling this forces sf::Text's lazy glyph layout/rasterization to run now.
				static_cast<void>(text.getLocalBounds());

				++combinationsTouched;
				if (progress && !progress(
					static_cast<float>(combinationsTouched) /
						static_cast<float>(TotalCombinationCount),
					"loading.preparing_text"))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool Run(Assets& assets, const LocalizationManager& localization,
		const ProgressCallback& progress)
	{
		const sf::String englishText{ localization.BuildWarmupText(Language::English) };
		sf::String localizedText;
		for (const Language language :
			{ Language::English, Language::Spanish, Language::Russian, Language::Ukrainian })
		{
			localizedText += localization.BuildWarmupText(language);
		}
		const sf::String arabicText{ localization.BuildWarmupText(Language::Arabic) };

		std::size_t combinationsTouched{ 0u };

		// MenuRegular/MenuSemibold are only ever selected while displaying
		// English with keepEnglishDisplayStyle=true; BodyRegular is English-only.
		if (!WarmupFont(assets.Fonts().Get(Config::Font::MenuSemibold), englishText, BoldSizes,
			progress, combinationsTouched))
			return false;
		if (!WarmupFont(assets.Fonts().Get(Config::Font::MenuRegular), englishText, RegularSizes,
			progress, combinationsTouched))
			return false;
		if (!WarmupFont(assets.Fonts().Get(Config::Font::BodyRegular), englishText, BodyRegularSizes,
			progress, combinationsTouched))
			return false;

		// LocalizedRegular/LocalizedBold are selected for every non-Arabic
		// language, including English when keepEnglishDisplayStyle=false.
		if (!WarmupFont(assets.Fonts().Get(Config::Font::LocalizedBold), localizedText, BoldSizes,
			progress, combinationsTouched))
			return false;
		if (!WarmupFont(assets.Fonts().Get(Config::Font::LocalizedRegular), localizedText, RegularSizes,
			progress, combinationsTouched))
			return false;

		if (!WarmupFont(assets.Fonts().Get(Config::Font::ArabicBold), arabicText, BoldSizes,
			progress, combinationsTouched))
			return false;
		if (!WarmupFont(assets.Fonts().Get(Config::Font::ArabicRegular), arabicText, RegularSizes,
			progress, combinationsTouched))
			return false;

		return true;
	}
}
