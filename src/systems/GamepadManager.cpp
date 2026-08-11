#include "GamepadManager.h"

#include <algorithm>
#include <cmath>

#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>

namespace
{
    constexpr unsigned int MicrosoftVendorId{ 0x045Eu };
    constexpr unsigned int SonyVendorId{ 0x054Cu };
    constexpr float StickDeadZone{ 0.18f };
    constexpr float TriggerThreshold{ -20.f };

    sf::Vector2f ApplyDeadZone(sf::Vector2f value)
    {
        value /= 100.f;
        const float magnitude{ std::sqrt(value.x * value.x + value.y * value.y) };
        if (magnitude <= StickDeadZone)
            return {};

        const float adjustedMagnitude{ std::clamp(
            (magnitude - StickDeadZone) / (1.f - StickDeadZone), 0.f, 1.f) };
        return value / magnitude * adjustedMagnitude;
    }
}

GamepadManager::GamepadManager()
{
    RefreshConnection();
}

void GamepadManager::HandleEvent(const sf::Event& event)
{
    if (event.is<sf::Event::JoystickConnected>() ||
        event.is<sf::Event::JoystickDisconnected>())
    {
        RefreshConnection();
    }

    if (event.is<sf::Event::MouseMoved>() ||
        event.is<sf::Event::MouseButtonPressed>() ||
        event.is<sf::Event::KeyPressed>())
    {
        usingGamepad = false;
    }
    else if (event.is<sf::Event::JoystickButtonPressed>())
    {
        usingGamepad = true;
    }
    else if (const auto* moved{ event.getIf<sf::Event::JoystickMoved>() })
    {
        if (IsActiveJoystick(moved->joystickId) && std::abs(moved->position) > 18.f)
            usingGamepad = true;
    }
}

GamepadManager::GameplayInput GamepadManager::GetGameplayInput() const
{
    if (!activeJoystick)
        return {};

    GameplayInput input;
    input.movement = ReadStick(false);

    const sf::Vector2f aim{ ReadStick(true) };
    const float aimLengthSquared{ aim.x * aim.x + aim.y * aim.y };
    if (aimLengthSquared > 0.f)
        input.aimDirection = aim / std::sqrt(aimLengthSquared);

    if (layout == Layout::PlayStation)
    {
        input.fire = IsButtonPressed(7u);
    }
    else
    {
        const unsigned int id{ *activeJoystick };
        input.fire = sf::Joystick::hasAxis(id, sf::Joystick::Axis::Z)
            ? sf::Joystick::getAxisPosition(id, sf::Joystick::Axis::Z) < TriggerThreshold
            : IsButtonPressed(7u);
    }

    return input;
}

bool GamepadManager::IsPausePressed(const sf::Event& event) const
{
    return GetNavigationAction(event) == NavigationAction::Pause;
}

GamepadManager::NavigationAction GamepadManager::GetNavigationAction(
    const sf::Event& event) const
{
    if (const auto* moved{ event.getIf<sf::Event::JoystickMoved>() })
    {
        if (!IsActiveJoystick(moved->joystickId))
            return NavigationAction::None;

        constexpr float DpadThreshold{ 50.f };
        if (moved->axis == sf::Joystick::Axis::PovX)
        {
            if (moved->position < -DpadThreshold)
                return NavigationAction::Left;
            if (moved->position > DpadThreshold)
                return NavigationAction::Right;
        }
        else if (moved->axis == sf::Joystick::Axis::PovY)
        {
            if (moved->position > DpadThreshold)
                return NavigationAction::Up;
            if (moved->position < -DpadThreshold)
                return NavigationAction::Down;
        }
        return NavigationAction::None;
    }

    const auto* pressed{ event.getIf<sf::Event::JoystickButtonPressed>() };
    if (pressed == nullptr || !IsActiveJoystick(pressed->joystickId))
        return NavigationAction::None;

    const unsigned int confirmButton{ layout == Layout::PlayStation ? 1u : 0u };
    const unsigned int backButton{ layout == Layout::PlayStation ? 2u : 1u };
    const unsigned int pauseButton{ layout == Layout::PlayStation ? 9u : 7u };
    if (pressed->button == confirmButton)
        return NavigationAction::Confirm;
    if (pressed->button == backButton)
        return NavigationAction::Back;
    if (pressed->button == pauseButton)
        return NavigationAction::Pause;
    return NavigationAction::None;
}

bool GamepadManager::IsConnected() const noexcept
{
    return activeJoystick.has_value();
}

bool GamepadManager::IsUsingGamepad() const noexcept
{
    return usingGamepad;
}

GamepadManager::Layout GamepadManager::GetLayout() const noexcept
{
    return layout;
}

void GamepadManager::RefreshConnection()
{
    activeJoystick.reset();
    layout = Layout::Generic;

    for (unsigned int id{ 0u }; id < sf::Joystick::Count; ++id)
    {
        if (!sf::Joystick::isConnected(id))
            continue;

        activeJoystick = id;
        const sf::Joystick::Identification identification{ sf::Joystick::getIdentification(id) };
        if (identification.vendorId == MicrosoftVendorId)
            layout = Layout::Xbox;
        else if (identification.vendorId == SonyVendorId)
            layout = Layout::PlayStation;
        return;
    }
}

bool GamepadManager::IsActiveJoystick(unsigned int joystickId) const noexcept
{
    return activeJoystick.has_value() && *activeJoystick == joystickId;
}

bool GamepadManager::IsButtonPressed(unsigned int button) const
{
    return activeJoystick.has_value() &&
        button < sf::Joystick::getButtonCount(*activeJoystick) &&
        sf::Joystick::isButtonPressed(*activeJoystick, button);
}

sf::Vector2f GamepadManager::ReadStick(bool aiming) const
{
    if (!activeJoystick)
        return {};

    const unsigned int id{ *activeJoystick };
    sf::Joystick::Axis horizontal{ sf::Joystick::Axis::X };
    sf::Joystick::Axis vertical{ sf::Joystick::Axis::Y };
    if (aiming)
    {
        if (layout == Layout::PlayStation)
        {
            horizontal = sf::Joystick::Axis::Z;
            vertical = sf::Joystick::Axis::R;
        }
        else
        {
            horizontal = sf::Joystick::Axis::U;
            vertical = sf::Joystick::Axis::V;
            if (!sf::Joystick::hasAxis(id, horizontal) ||
                !sf::Joystick::hasAxis(id, vertical))
            {
                horizontal = sf::Joystick::Axis::Z;
                vertical = sf::Joystick::Axis::R;
            }
        }
    }

    if (!sf::Joystick::hasAxis(id, horizontal) || !sf::Joystick::hasAxis(id, vertical))
        return {};

    return ApplyDeadZone({
        sf::Joystick::getAxisPosition(id, horizontal),
        sf::Joystick::getAxisPosition(id, vertical) });
}
