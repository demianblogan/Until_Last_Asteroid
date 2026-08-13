#pragma once

#include <optional>
#include <string>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "settings/GameSettings.h"
#include "ui/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class AssetStore;

namespace sf
{
    class RenderTarget;
}

class TutorialDirector
{
public:
    enum class Action
    {
        SpawnBigMeteor,
        HighlightScore,
        HighlightArmor,
        HighlightShield,
        SpawnShooter,
        SpawnShield,
        Complete
    };

    struct Snapshot
    {
        sf::Vector2f playerPosition;
        unsigned int playerAttacksFired{ 0u };
        unsigned int bigMeteorsDestroyed{ 0u };
        unsigned int smallMeteorsDestroyed{ 0u };
        unsigned int shootersDestroyed{ 0u };
        unsigned int shieldPickupsCollected{ 0u };
    };

    TutorialDirector(
        AssetStore& assets,
        sf::Vector2f logicalSize,
        const ControlSettings& controls);

    void Start(const Snapshot& snapshot);
    [[nodiscard]] std::optional<Action> Update(float deltaTime, const Snapshot& snapshot);
    void Draw(sf::RenderTarget& target);
    [[nodiscard]] bool IsActive() const noexcept;

private:
    enum class Step
    {
        Movement,
        Fire,
        BigAsteroid,
        Fragments,
        Score,
        Armor,
        Enemies,
        Shooter,
        Collision,
        ShieldPickup,
        ShieldInfo,
        Finish,
        Complete
    };

    void RequestStep(Step nextStep);
    [[nodiscard]] std::optional<Action> EnterStep(Step nextStep, const Snapshot& snapshot);
    void SetInstruction(std::string instruction);
    [[nodiscard]] std::optional<Action> UpdateCurrentStep(
        float deltaTime,
        const Snapshot& snapshot);
    [[nodiscard]] static std::string BindingName(const ControlBinding& binding);
    [[nodiscard]] static float DistanceSquared(sf::Vector2f first, sf::Vector2f second) noexcept;

    static constexpr float HoldAfterInput{ 5.f };
    static constexpr float StandardMessageDuration{ 5.f };
    static constexpr float ShieldMessageDuration{ 5.f };
    static constexpr float SlideSpeed{ 4.5f };

    sf::Vector2f logicalSize;
    RoundedRectangleShape panel;
    sf::Text text;
    NeonGlow glow;
    std::string movementInstruction;
    std::string fireInstruction;
    Step step{ Step::Movement };
    Step pendingStep{ Step::Movement };
    sf::Vector2f movementStart;
    unsigned int shotBaseline{ 0u };
    unsigned int bigMeteorBaseline{ 0u };
    unsigned int smallMeteorBaseline{ 0u };
    unsigned int shooterBaseline{ 0u };
    unsigned int shieldPickupBaseline{ 0u };
    float visibleAmount{ 0.f };
    float stepElapsed{ 0.f };
    bool conditionMet{ false };
    bool hidingCurrentStep{ false };
    bool switchingStep{ false };
    bool active{ false };
};
