#pragma once

#include "core/Entity.h"
#include "game/GameplayData.h"

class GameplaySession;

class Pickup final : public Entity
{
public:
    enum class Kind
    {
        Health,
        Shield
    };

    Pickup(AssetStore& assets, World& world, Kind kind);

    [[nodiscard]] Type GetType() const noexcept override;
    [[nodiscard]] bool IsCollideWith(const Entity& other) const override;
    void Update(float deltaTime) override;
    [[nodiscard]] bool Apply(GameplaySession& session);
    [[nodiscard]] Kind GetKind() const noexcept;

private:
    Kind kind;
    const GameplayData::PickupConfig& config;
};
