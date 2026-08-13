#include "TutorialDirector.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr sf::Vector2f PanelSize{ 1078.f, 168.f };
    constexpr float VisibleY{ 32.f };
    constexpr float HiddenY{ -195.f };
    constexpr sf::Color PanelColor{ 3, 18, 34, 238 };
    constexpr sf::Color Cyan{ 35, 225, 255 };

    void CenterText(sf::Text& text, sf::Vector2f position)
    {
        const sf::FloatRect bounds{ text.getLocalBounds() };
        text.setOrigin({
            bounds.position.x + bounds.size.x * 0.5f,
            bounds.position.y + bounds.size.y * 0.5f });
        text.setPosition(position);
    }

    std::string MouseButtonName(sf::Mouse::Button button)
    {
        switch (button)
        {
        case sf::Mouse::Button::Left: return "Left Mouse Button";
        case sf::Mouse::Button::Right: return "Right Mouse Button";
        case sf::Mouse::Button::Middle: return "Middle Mouse Button";
        case sf::Mouse::Button::Extra1: return "Mouse Button 4";
        case sf::Mouse::Button::Extra2: return "Mouse Button 5";
        }
        return "Mouse Button";
    }
}

TutorialDirector::TutorialDirector(
    AssetStore& assets,
    sf::Vector2f size,
    const ControlSettings& controls)
    : logicalSize(size)
    , panel(PanelSize, 18.f, 10u)
    , text(assets.Fonts().Get(Config::Font::BodyRegular), "", 27)
    , glow(assets)
{
    panel.setFillColor(PanelColor);
    panel.setOutlineColor(Cyan);
    panel.setOutlineThickness(2.f);
    text.setFillColor(sf::Color(218, 249, 255));
    text.setOutlineColor(sf::Color(0, 8, 18, 230));
    text.setOutlineThickness(2.f);
    text.setLineSpacing(1.16f);

    movementInstruction = "Press " + BindingName(controls.moveUp) + ", " +
        BindingName(controls.moveLeft) + ", " + BindingName(controls.moveDown) +
        ", or " + BindingName(controls.moveRight) + " to move your ship.\n"
        "You can also use the Left Stick on your gamepad.";
    fireInstruction = "Hold " + BindingName(controls.fire) +
        " or the Right Trigger on your gamepad to fire.";
}

void TutorialDirector::Start(const Snapshot& snapshot)
{
    active = true;
    switchingStep = false;
    visibleAmount = 0.f;
    stepElapsed = 0.f;
    conditionMet = false;
    static_cast<void>(EnterStep(Step::Movement, snapshot));
}

std::optional<TutorialDirector::Action> TutorialDirector::Update(
    float deltaTime,
    const Snapshot& snapshot)
{
    if (!active || deltaTime <= 0.f)
        return std::nullopt;

    glow.Update(deltaTime);
    glow.Invalidate();

    if (switchingStep)
    {
        visibleAmount = std::max(0.f, visibleAmount - deltaTime * SlideSpeed);
        if (visibleAmount <= 0.f)
        {
            switchingStep = false;
            return EnterStep(pendingStep, snapshot);
        }
        return std::nullopt;
    }

    if (hidingCurrentStep)
    {
        visibleAmount = std::max(0.f, visibleAmount - deltaTime * SlideSpeed);
        return UpdateCurrentStep(deltaTime, snapshot);
    }

    visibleAmount = std::min(1.f, visibleAmount + deltaTime * SlideSpeed);
    if (visibleAmount < 1.f)
        return std::nullopt;

    return UpdateCurrentStep(deltaTime, snapshot);
}

void TutorialDirector::Draw(sf::RenderTarget& target)
{
    if (!active || visibleAmount <= 0.f)
        return;

    const float eased{ visibleAmount * visibleAmount * (3.f - 2.f * visibleAmount) };
    const float panelY{ std::lerp(HiddenY, VisibleY, eased) };
    const sf::Vector2f panelPosition{ (logicalSize.x - PanelSize.x) * 0.5f, panelY };
    panel.setPosition(panelPosition);
    CenterText(text, panelPosition + PanelSize * 0.5f);

    glow.DrawBloom(
        target,
        panel.getGlobalBounds(),
        [this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
        {
            glowTarget.draw(panel, states);
            glowTarget.draw(text, states);
        },
        Cyan,
        false);
    target.draw(panel);
    target.draw(text);
    glow.DrawHighlight(target, panel.getGlobalBounds(), Cyan);
}

bool TutorialDirector::IsActive() const noexcept
{
    return active;
}

void TutorialDirector::RequestStep(Step nextStep)
{
    pendingStep = nextStep;
    switchingStep = true;
}

std::optional<TutorialDirector::Action> TutorialDirector::EnterStep(
    Step nextStep,
    const Snapshot& snapshot)
{
    step = nextStep;
    stepElapsed = 0.f;
    conditionMet = false;
    hidingCurrentStep = false;

    switch (step)
    {
    case Step::Movement:
        movementStart = snapshot.playerPosition;
        SetInstruction(movementInstruction);
        break;
    case Step::Fire:
        shotBaseline = snapshot.playerAttacksFired;
        SetInstruction(fireInstruction);
        break;
    case Step::BigAsteroid:
        bigMeteorBaseline = snapshot.bigMeteorsDestroyed;
        smallMeteorBaseline = snapshot.smallMeteorsDestroyed;
        SetInstruction("An asteroid has appeared ahead of you. Destroy it.");
        return Action::SpawnBigMeteor;
    case Step::Fragments:
        SetInstruction("Large asteroids split into smaller fragments when destroyed.\n"
            "Destroy both fragments.");
        break;
    case Step::Score:
        SetInstruction("Destroying asteroids and enemies awards points.\nYour total score is displayed in the upper-left corner.");
        return Action::HighlightScore;
    case Step::Armor:
        SetInstruction("Your armor is displayed in the lower-left corner.\nIf it reaches 0%, your ship will be destroyed.");
        return Action::HighlightArmor;
    case Step::Enemies:
        SetInstruction("Asteroids are not the only threat.\nEnemy ships will hunt you and try to destroy your ship.");
        break;
    case Step::Shooter:
        shooterBaseline = snapshot.shootersDestroyed;
        SetInstruction("An enemy gunship has appeared. Destroy it and evade its attacks.");
        return Action::SpawnShooter;
    case Step::Collision:
        SetInstruction("Be careful when touching asteroids or enemy ships.\nCollisions damage both your ship and the enemy.");
        break;
    case Step::ShieldPickup:
        shieldPickupBaseline = snapshot.shieldPickupsCollected;
        SetInstruction("Defeated enemies can drop useful bonuses.\nCollect the shield bonus that has appeared.");
        return Action::SpawnShield;
    case Step::ShieldInfo:
        SetInstruction("You collected a shield. Its meter is shown above your armor.\n"
            "It absorbs damage, but its energy continuously drains.");
        return Action::HighlightShield;
    case Step::Finish:
        SetInstruction("Tutorial complete. Good luck on your adventure!");
        break;
    case Step::Complete:
        active = false;
        return Action::Complete;
    }
    return std::nullopt;
}

void TutorialDirector::SetInstruction(std::string instruction)
{
    text.setString(std::move(instruction));
    CenterText(text, { logicalSize.x * 0.5f, VisibleY + PanelSize.y * 0.5f });
    glow.Invalidate();
}

std::optional<TutorialDirector::Action> TutorialDirector::UpdateCurrentStep(
    float deltaTime,
    const Snapshot& snapshot)
{
    stepElapsed += deltaTime;
    switch (step)
    {
    case Step::Movement:
		if (!conditionMet && DistanceSquared(snapshot.playerPosition, movementStart) > 400.f)
		{
			conditionMet = true;
			stepElapsed = 0.f;
		}
        if (conditionMet && stepElapsed >= HoldAfterInput)
            RequestStep(Step::Fire);
        break;
    case Step::Fire:
        if (!conditionMet && snapshot.playerAttacksFired > shotBaseline)
        {
            conditionMet = true;
            stepElapsed = 0.f;
        }
        if (conditionMet && stepElapsed >= HoldAfterInput)
            RequestStep(Step::BigAsteroid);
        break;
    case Step::BigAsteroid:
        if (stepElapsed >= StandardMessageDuration)
            hidingCurrentStep = true;
        if (stepElapsed >= StandardMessageDuration &&
            snapshot.bigMeteorsDestroyed > bigMeteorBaseline)
            RequestStep(Step::Fragments);
        break;
    case Step::Fragments:
        if (stepElapsed >= StandardMessageDuration &&
            snapshot.smallMeteorsDestroyed >= smallMeteorBaseline + 2u)
            RequestStep(Step::Score);
        break;
    case Step::Score:
        if (stepElapsed >= StandardMessageDuration) RequestStep(Step::Armor);
        break;
    case Step::Armor:
        if (stepElapsed >= StandardMessageDuration) RequestStep(Step::Enemies);
        break;
    case Step::Enemies:
        if (stepElapsed >= StandardMessageDuration) RequestStep(Step::Shooter);
        break;
    case Step::Shooter:
        if (stepElapsed >= StandardMessageDuration &&
            snapshot.shootersDestroyed > shooterBaseline)
            RequestStep(Step::Collision);
        break;
    case Step::Collision:
        if (stepElapsed >= StandardMessageDuration) RequestStep(Step::ShieldPickup);
        break;
    case Step::ShieldPickup:
        if (snapshot.shieldPickupsCollected > shieldPickupBaseline)
            RequestStep(Step::ShieldInfo);
        break;
    case Step::ShieldInfo:
        if (stepElapsed >= ShieldMessageDuration) RequestStep(Step::Finish);
        break;
    case Step::Finish:
        if (stepElapsed >= StandardMessageDuration) RequestStep(Step::Complete);
        break;
    case Step::Complete:
        break;
    }
    return std::nullopt;
}

std::string TutorialDirector::BindingName(const ControlBinding& binding)
{
    if (binding.device == InputDevice::Mouse)
        return MouseButtonName(static_cast<sf::Mouse::Button>(binding.code));

    const auto key{ static_cast<sf::Keyboard::Key>(binding.code) };
    return sf::Keyboard::getDescription(sf::Keyboard::delocalize(key)).toAnsiString();
}

float TutorialDirector::DistanceSquared(sf::Vector2f first, sf::Vector2f second) noexcept
{
    const sf::Vector2f delta{ first - second };
    return delta.x * delta.x + delta.y * delta.y;
}
