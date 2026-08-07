#pragma once

#include <SFML/Graphics/Sprite.hpp>

#include "states/State.h"

namespace sf
{
    class Event;
    class Music;
}

class CompanySplashState final : public State
{
public:
    CompanySplashState(StateStack& stateStack, StateContext context);
    ~CompanySplashState() override;

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    [[nodiscard]] static bool IsSkipEvent(const sf::Event& event);
    void Finish();
    void UpdateOpacity();

    static constexpr float FADE_IN_DURATION{ 0.5f };
    static constexpr float HOLD_DURATION{ 2.f };
    static constexpr float FADE_OUT_DURATION{ 0.5f };

    sf::Sprite logo;
    sf::Music& splashMusic;
    float elapsedTime{ 0.f };
    bool isFinishing{ false };
};
