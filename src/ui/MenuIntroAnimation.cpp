#include "MenuIntroAnimation.h"

#include <algorithm>
#include <utility>

MenuIntroAnimation::MenuIntroAnimation(sf::String titleText, std::vector<sf::String> itemTexts)
    : title(std::move(titleText))
    , menuItems(std::move(itemTexts))
    , visibleMenuCharacters(menuItems.size(), 0)
{
}

MenuIntroAnimation::Events MenuIntroAnimation::Update(float deltaTime)
{
    Events events;

    switch (phase)
    {
    case Phase::TypingTitle:
        UpdateTypingTitle(deltaTime, events);
        break;

    case Phase::MovingTitle:
        UpdateMovingTitle(deltaTime);
        break;

    case Phase::TypingMenuItems:
        UpdateTypingMenuItems(deltaTime, events);
        break;

    case Phase::RevealingFrames:
        phaseTimer += deltaTime;
        frameOpacity = std::clamp(phaseTimer / FRAME_REVEAL_DURATION, 0.f, 1.f);
        if (frameOpacity >= 1.f)
        {
            phase = Phase::Interactive;
            events.becameInteractive = true;
        }
        break;

    case Phase::Interactive:
        break;
    }

    return events;
}

MenuIntroAnimation::Events MenuIntroAnimation::Skip()
{
    Events events;
    if (phase == Phase::Interactive)
        return events;

    visibleTitleCharacters = title.getSize();
    for (std::size_t index{ 0 }; index < menuItems.size(); ++index)
        visibleMenuCharacters[index] = menuItems[index].getSize();

    titleMoveProgress = 1.f;
    frameOpacity = 1.f;
    phase = Phase::Interactive;
    events.activationStarted = !hasStartedActivation;
    events.becameInteractive = true;
    hasStartedActivation = true;
    return events;
}

sf::String MenuIntroAnimation::GetVisibleTitle() const
{
    return title.substring(0, visibleTitleCharacters);
}

sf::String MenuIntroAnimation::GetVisibleMenuItem(std::size_t index) const
{
    if (index >= menuItems.size())
        return {};

    return menuItems[index].substring(0, visibleMenuCharacters[index]);
}

float MenuIntroAnimation::GetTitleMoveProgress() const noexcept
{
    return titleMoveProgress;
}

float MenuIntroAnimation::GetFrameOpacity() const noexcept
{
    return frameOpacity;
}

bool MenuIntroAnimation::IsInteractive() const noexcept
{
    return phase == Phase::Interactive;
}

void MenuIntroAnimation::UpdateTypingTitle(float deltaTime, Events& events)
{
    characterTimer += deltaTime;

    while (characterTimer >= TITLE_CHARACTER_INTERVAL && visibleTitleCharacters < title.getSize())
    {
        characterTimer -= TITLE_CHARACTER_INTERVAL;
        if (title[visibleTitleCharacters] != U' ')
            ++events.typedCharacters;

        ++visibleTitleCharacters;
    }

    if (visibleTitleCharacters >= title.getSize())
    {
        phase = Phase::MovingTitle;
        phaseTimer = 0.f;
        characterTimer = 0.f;
    }
}

void MenuIntroAnimation::UpdateMovingTitle(float deltaTime)
{
    phaseTimer += deltaTime;
    const float linearProgress{ std::clamp(phaseTimer / TITLE_MOVE_DURATION, 0.f, 1.f) };
    const float inverse{ 1.f - linearProgress };
    titleMoveProgress = 1.f - inverse * inverse * inverse;

    if (linearProgress >= 1.f)
    {
        phase = Phase::TypingMenuItems;
        phaseTimer = 0.f;
        characterTimer = 0.f;
    }
}

void MenuIntroAnimation::UpdateTypingMenuItems(float deltaTime, Events& events)
{
    if (currentMenuItem >= menuItems.size())
    {
        StartFrameReveal(events);
        return;
    }

    const sf::String& item{ menuItems[currentMenuItem] };
    std::size_t& visibleCharacters{ visibleMenuCharacters[currentMenuItem] };

    if (visibleCharacters < item.getSize())
    {
        characterTimer += deltaTime;

        while (characterTimer >= MENU_CHARACTER_INTERVAL && visibleCharacters < item.getSize())
        {
            characterTimer -= MENU_CHARACTER_INTERVAL;
            if (item[visibleCharacters] != U' ')
                ++events.typedCharacters;

            ++visibleCharacters;
        }

        if (visibleCharacters >= item.getSize())
        {
            phaseTimer = 0.f;
            if (currentMenuItem + 1 >= menuItems.size())
                StartFrameReveal(events);
        }

        return;
    }

    phaseTimer += deltaTime;
    if (phaseTimer >= MENU_ITEM_PAUSE)
    {
        ++currentMenuItem;
        phaseTimer = 0.f;
        characterTimer = 0.f;
    }
}

void MenuIntroAnimation::StartFrameReveal(Events& events)
{
    phase = Phase::RevealingFrames;
    phaseTimer = 0.f;
    frameOpacity = 0.f;
    hasStartedActivation = true;
    events.activationStarted = true;
}
