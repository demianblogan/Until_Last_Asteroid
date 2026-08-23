#pragma once

#include <cstddef>
#include <vector>

#include <SFML/System/String.hpp>

class MenuIntroAnimation
{
public:
    struct Events
    {
        std::size_t typedCharacters{ 0 };
        bool activationStarted{ false };
        bool becameInteractive{ false };
    };

    MenuIntroAnimation(sf::String title, std::vector<sf::String> menuItems);

    [[nodiscard]] Events Update(float deltaTime);
    [[nodiscard]] Events Skip();

    [[nodiscard]] sf::String GetVisibleTitle() const;
    [[nodiscard]] sf::String GetVisibleMenuItem(std::size_t index) const;
    [[nodiscard]] float GetTitleMoveProgress() const noexcept;
    [[nodiscard]] float GetFrameOpacity() const noexcept;
    [[nodiscard]] bool IsInteractive() const noexcept;

private:
    enum class Phase
    {
        TypingTitle,
        MovingTitle,
        TypingMenuItems,
        RevealingFrames,
        Interactive
    };

    void UpdateTypingTitle(float deltaTime, Events& events);
    void UpdateMovingTitle(float deltaTime);
    void UpdateTypingMenuItems(float deltaTime, Events& events);
    void StartFrameReveal(Events& events);

    static constexpr float TITLE_CHARACTER_INTERVAL{ 0.045f };
    static constexpr float TITLE_MOVE_DURATION{ 0.65f };
    static constexpr float MENU_CHARACTER_INTERVAL{ 0.035f };
    static constexpr float MENU_ITEM_PAUSE{ 0.06f };
    static constexpr float FRAME_REVEAL_DURATION{ 0.28f };

    sf::String title;
    std::vector<sf::String> menuItems;
    std::vector<std::size_t> visibleMenuCharacters;

    Phase phase{ Phase::TypingTitle };
    std::size_t visibleTitleCharacters{ 0 };
    std::size_t currentMenuItem{ 0 };
    float characterTimer{ 0.f };
    float phaseTimer{ 0.f };
    float titleMoveProgress{ 0.f };
    float frameOpacity{ 0.f };
    bool hasStartedActivation{ false };
};
