#include "WorldCampaignPickupQueue.h"

void WorldCampaignPickupQueue::Configure(const std::vector<GameplayData::PickupKind>& newSequence)
{
	sequence = newSequence;
	nextIndex = 0u;
}

void WorldCampaignPickupQueue::Clear()
{
	sequence.clear();
	nextIndex = 0u;
}

std::optional<GameplayData::PickupKind> WorldCampaignPickupQueue::TryPop()
{
	if (nextIndex >= sequence.size())
		return std::nullopt;
	else
		return sequence[nextIndex++];
}