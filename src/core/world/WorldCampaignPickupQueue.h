#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "gameplay/GameplayData.h"

// A pre-scripted sequence of campaign reward pickups (set once per level via
// Configure), handed out one at a time as enemies drop rewards. Once
// exhausted, TryPop returns nullopt and callers fall back to random drops.
class WorldCampaignPickupQueue
{
public:
	void Configure(const std::vector<GameplayData::PickupKind>& sequence);
	void Clear();

	[[nodiscard]] std::optional<GameplayData::PickupKind> TryPop();

private:
	std::vector<GameplayData::PickupKind> sequence;
	std::size_t nextIndex = 0u;
};
