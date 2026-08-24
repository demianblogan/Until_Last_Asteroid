#include "TutorialDirector.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "localization/LocalizationManager.h"
#include "ui/TextLayout.h"
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

}

TutorialDirector::TutorialDirector(
    Assets& assets,
    LocalizationManager& localizationManager,
    sf::Vector2f size,
    const ControlSettings& controls)
    : logicalSize(size), localization(localizationManager)
    , panel(PanelSize, 18.f, 10u)
    , text(assets.Fonts().Get(localizationManager.GetRegularFont(false)), "", 27)
    , glow(assets)
{
    panel.setFillColor(PanelColor);
    panel.setOutlineColor(Cyan);
    panel.setOutlineThickness(2.f);
    text.setFillColor(sf::Color(218, 249, 255));
    text.setOutlineColor(sf::Color(0, 8, 18, 230));
    text.setOutlineThickness(2.f);
    text.setLineSpacing(1.16f);

	const sf::String bindings{ BindingName(controls.moveUp) + sf::String(", ") +
		BindingName(controls.moveLeft) + sf::String(", ") + BindingName(controls.moveDown) +
		sf::String(", ") + BindingName(controls.moveRight) };
	const sf::U8String bindingBytes{ bindings.toUtf8() };
	const sf::U8String fireBytes{ BindingName(controls.fire).toUtf8() };
    movementInstruction = localization.FormatText("tutorial.movement", "bindings",
		std::string(reinterpret_cast<const char*>(bindingBytes.data()), bindingBytes.size()));
    fireInstruction = localization.FormatText("tutorial.fire", "binding",
		std::string(reinterpret_cast<const char*>(fireBytes.data()), fireBytes.size()));
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
		SetInstruction(localization.GetText("tutorial.asteroid"));
        return Action::SpawnBigMeteor;
    case Step::Fragments:
		SetInstruction(localization.GetText("tutorial.fragments"));
        break;
    case Step::Score:
		SetInstruction(localization.GetText("tutorial.score"));
        return Action::HighlightScore;
    case Step::Armor:
		SetInstruction(localization.GetText("tutorial.armor"));
        return Action::HighlightArmor;
    case Step::Enemies:
		SetInstruction(localization.GetText("tutorial.enemies"));
        break;
    case Step::Shooter:
        shooterBaseline = snapshot.shootersDestroyed;
		SetInstruction(localization.GetText("tutorial.shooter"));
        return Action::SpawnShooter;
    case Step::Collision:
		SetInstruction(localization.GetText("tutorial.collision"));
        break;
    case Step::ShieldPickup:
        shieldPickupBaseline = snapshot.shieldPickupsCollected;
		SetInstruction(localization.GetText("tutorial.shield_pickup"));
        return Action::SpawnShield;
    case Step::ShieldInfo:
		SetInstruction(localization.GetText("tutorial.shield_info"));
        return Action::HighlightShield;
    case Step::PartPickup:
        partsBaseline = snapshot.partsCollected;
		SetInstruction(localization.GetText("tutorial.part_pickup"));
        return Action::SpawnPart;
    case Step::PartInfo:
		SetInstruction(localization.GetText("tutorial.part_info"));
        return Action::HighlightParts;
    case Step::Finish:
		SetInstruction(localization.GetText("tutorial.complete"));
        break;
    case Step::Complete:
        active = false;
        return Action::Complete;
    }
    return std::nullopt;
}

void TutorialDirector::SetInstruction(const sf::String& instruction)
{
	text.setCharacterSize(27u);
	text.setString(instruction);
	TextLayout::FitWidth(text, PanelSize.x - 80.f, 20u);
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
        if (stepElapsed >= ShieldMessageDuration) RequestStep(Step::PartPickup);
        break;
    case Step::PartPickup:
        if (snapshot.partsCollected > partsBaseline)
            RequestStep(Step::PartInfo);
        else if (stepElapsed >= 5.5f)
        {
            stepElapsed = 0.f;
            return Action::SpawnPart;
        }
        break;
    case Step::PartInfo:
        if (stepElapsed >= StandardMessageDuration) RequestStep(Step::Finish);
        break;
    case Step::Finish:
        if (stepElapsed >= StandardMessageDuration) RequestStep(Step::Complete);
        break;
    case Step::Complete:
        break;
    }
    return std::nullopt;
}

sf::String TutorialDirector::BindingName(const ControlBinding& binding) const
{
    if (binding.device == RebindableInputDevice::Mouse)
	{
		switch (static_cast<sf::Mouse::Button>(binding.code))
		{
		case sf::Mouse::Button::Left: return localization.GetText("options.mouse_left");
		case sf::Mouse::Button::Right: return localization.GetText("options.mouse_right");
		case sf::Mouse::Button::Middle: return localization.GetText("options.mouse_middle");
		case sf::Mouse::Button::Extra1: return localization.GetText("options.mouse_4");
		case sf::Mouse::Button::Extra2: return localization.GetText("options.mouse_5");
		}
		return localization.GetText("options.mouse");
	}

    const auto key{ static_cast<sf::Keyboard::Key>(binding.code) };
    return sf::Keyboard::getDescription(sf::Keyboard::delocalize(key));
}

float TutorialDirector::DistanceSquared(sf::Vector2f first, sf::Vector2f second) noexcept
{
    const sf::Vector2f delta{ first - second };
    return delta.x * delta.x + delta.y * delta.y;
}
