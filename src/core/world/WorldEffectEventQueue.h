#pragma once

#include <vector>

#include "rendering/EffectEvent.h"

// Buffers one-shot visual-effect events (hit sparks, explosions...) raised
// during Update, for the renderer to consume and clear once per frame.
class WorldEffectEventQueue
{
public:
	WorldEffectEventQueue();

	void Add(const EffectEvent& event);
	[[nodiscard]] const std::vector<EffectEvent>& Get() const noexcept;
	void Clear() noexcept;

private:
	std::vector<EffectEvent> events;
};
