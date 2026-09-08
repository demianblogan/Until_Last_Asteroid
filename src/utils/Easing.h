#pragma once

#include <algorithm>

// Shared easing curves for animations (panel slides, fades, camera moves,
// boss arrival...). Kept in one place so the same hand-rolled polynomial
// isn't copy-pasted into every screen that needs a soft start/stop.
namespace Easing
{
	// The classic Hermite "smoothstep": remaps a 0..1 progress value onto a
	// 0..1 curve that leaves and arrives with zero slope, so whatever it
	// drives (position, alpha, scale) accelerates in and decelerates out
	// instead of snapping. Input is clamped, so callers don't have to guard
	// against a progress value that has slightly overshot its range.
	[[nodiscard]] constexpr float SmoothStep(float progress) noexcept
	{
		progress = std::clamp(progress, 0.f, 1.f);
		return progress * progress * (3.f - 2.f * progress);
	}
}
