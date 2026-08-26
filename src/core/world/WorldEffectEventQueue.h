#pragma once

#include <vector>

#include "rendering/EffectEvent.h"

// Buffers one-shot visual-effect events (hit sparks, explosions...) raised
// during Update, for the renderer to consume and clear once per frame.
class WorldEffectEventQueue
{
public:
	WorldEffectEventQueue();

	void Add(const Rendering::EffectEvent& event);
	[[nodiscard]] const std::vector<Rendering::EffectEvent>& Get() const noexcept;
	void Clear() noexcept;

private:
	std::vector<Rendering::EffectEvent> events;
};
