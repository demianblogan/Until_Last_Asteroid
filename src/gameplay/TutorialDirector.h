#pragma once

#include <optional>
#include <string>

#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "settings/GameSettings.h"
#include "ui/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class Assets;
class LocalizationManager;

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
        HighlightParts,
        SpawnShooter,
        SpawnShield,
        SpawnPart,
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
        int partsCollected{ 0 };
    };

    TutorialDirector(
        Assets& assets,
		LocalizationManager& localization,
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
        PartPickup,
        PartInfo,
        Finish,
        Complete
    };

    void RequestStep(Step nextStep);
    [[nodiscard]] std::optional<Action> EnterStep(Step nextStep, const Snapshot& snapshot);
    void SetInstruction(const sf::String& instruction);
    [[nodiscard]] std::optional<Action> UpdateCurrentStep(
        float deltaTime,
        const Snapshot& snapshot);
    [[nodiscard]] sf::String BindingName(const ControlBinding& binding) const;
    [[nodiscard]] static float DistanceSquared(sf::Vector2f first, sf::Vector2f second) noexcept;

    static constexpr float HoldAfterInput{ 5.f };
    static constexpr float StandardMessageDuration{ 5.f };
    static constexpr float ShieldMessageDuration{ 5.f };
    static constexpr float SlideSpeed{ 4.5f };

    sf::Vector2f logicalSize;
	LocalizationManager& localization;
    RoundedRectangleShape panel;
    sf::Text text;
    NeonGlow glow;
    sf::String movementInstruction;
    sf::String fireInstruction;
    Step step{ Step::Movement };
    Step pendingStep{ Step::Movement };
    sf::Vector2f movementStart;
    unsigned int shotBaseline{ 0u };
    unsigned int bigMeteorBaseline{ 0u };
    unsigned int smallMeteorBaseline{ 0u };
    unsigned int shooterBaseline{ 0u };
    unsigned int shieldPickupBaseline{ 0u };
    int partsBaseline{ 0 };
    float visibleAmount{ 0.f };
    float stepElapsed{ 0.f };
    bool conditionMet{ false };
    bool hidingCurrentStep{ false };
    bool switchingStep{ false };
    bool active{ false };
};
