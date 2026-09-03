#pragma once

#include "core/Entity.h"
#include "gameplay/GameplayData.h"

class GameplaySession;

// A stationary collectible drop (health, shield, a weapon bonus, or a
// HelperBot companion -- see Kind). Doesn't move or animate on its own;
// Apply() is called once the player actually touches it, applying whatever
// effect that Kind grants to the current GameplaySession and reporting
// whether it was actually used (health pickups can be rejected at full HP).
class Pickup final : public Entity
{
public:
	using Kind = GameplayData::PickupKind;

	Pickup(Assets& assets, World& world, Kind kind);

	[[nodiscard]] Type GetType() const noexcept override;
	[[nodiscard]] bool IsCollidingWith(const Entity& other) const override;
	void Update(float) override;

	[[nodiscard]] bool Apply(GameplaySession& session);
	[[nodiscard]] Kind GetKind() const noexcept;

private:
	Kind kind;
	const GameplayData::PickupConfig& config;
};