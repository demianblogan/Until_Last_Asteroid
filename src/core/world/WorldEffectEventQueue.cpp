#include "WorldEffectEventQueue.h"

namespace
{
	// A rough estimate of how many effect events a busy frame can raise, just to
	// avoid a few reallocations -- not a hard limit, the queue can grow past this.
	constexpr std::size_t EstimatedEventsPerFrame = 256u;
}

WorldEffectEventQueue::WorldEffectEventQueue()
{
	events.reserve(EstimatedEventsPerFrame);
}

void WorldEffectEventQueue::Add(const Rendering::EffectEvent& event)
{
	events.push_back(event);
}

const std::vector<Rendering::EffectEvent>& WorldEffectEventQueue::Get() const noexcept
{
	return events;
}

void WorldEffectEventQueue::Clear() noexcept
{
	events.clear();
}