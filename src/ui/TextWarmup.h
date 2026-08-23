#pragma once

#include <functional>
#include <string_view>

class Assets;
class LocalizationManager;

// Forces every (font, character size) combination actually used anywhere in
// the game through its first, expensive sf::Text layout pass up front,
// instead of paying that cost piecemeal the first time each individual
// screen happens to appear. Meant to run once during the (already
// backgrounded) asset loading, on a thread holding an active GL context.
namespace TextWarmup
{
	// Same shape as Assets::ProgressCallback (progress fraction in, "keep
	// going?" bool out), kept as an independent alias so this module doesn't
	// need to depend on Assets's own type just to describe a callback shape.
	using ProgressCallback = std::function<bool(float, std::string_view)>;

	// Returns false if the callback requested cancellation partway through.
	[[nodiscard]] bool Run(Assets& assets, const LocalizationManager& localization,
		const ProgressCallback& progress = {});
}
