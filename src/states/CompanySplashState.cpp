#include "CompanySplashState.h"

#include <algorithm>
#include <cstdint>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "assets/AssetStore.h"
#include "states/StateId.h"
#include "utils/ConfigEnums.h"

CompanySplashState::CompanySplashState(StateStack& stateStack, StateContext context)
    : State(stateStack, context)
    , logo(context.assets.Textures().Get(Config::Texture::CompanyLogo))
    , splashMusic(context.assets.Music().Get(Config::Music::CompanySplash))
{
    context.window.setMouseCursorVisible(false);

    const sf::Vector2u textureSize{ logo.getTexture().getSize() };
    const float scale{ std::min(
        context.logicalSize.x * 0.8f / static_cast<float>(textureSize.x),
        context.logicalSize.y * 0.8f / static_cast<float>(textureSize.y)) };

    logo.setOrigin({
        static_cast<float>(textureSize.x) * 0.5f,
        static_cast<float>(textureSize.y) * 0.5f
    });
    logo.setScale({ scale, scale });
    logo.setPosition(context.logicalSize * 0.5f);
    logo.setColor(sf::Color(255, 255, 255, 0));

    splashMusic.play();
}

CompanySplashState::~CompanySplashState()
{
    splashMusic.stop();
}

void CompanySplashState::HandleEvent(const sf::Event& event)
{
    if (IsSkipEvent(event))
        Finish();
}

void CompanySplashState::Update(float deltaTime)
{
    if (isFinishing)
        return;

    elapsedTime += deltaTime;
    UpdateOpacity();

    if (elapsedTime >= FADE_IN_DURATION + HOLD_DURATION + FADE_OUT_DURATION)
        Finish();
}

void CompanySplashState::Render()
{
    GetContext().window.draw(logo);
}

bool CompanySplashState::IsSkipEvent(const sf::Event& event)
{
    // Future controller support can be added here without changing transition logic.
    return event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>();
}

void CompanySplashState::Finish()
{
    if (isFinishing)
        return;

    isFinishing = true;
    splashMusic.stop();
    RequestClear();
    RequestPush(StateId::MainMenu);
}

void CompanySplashState::UpdateOpacity()
{
    float opacity{ 1.f };

    if (elapsedTime < FADE_IN_DURATION)
    {
        opacity = elapsedTime / FADE_IN_DURATION;
    }
    else if (elapsedTime > FADE_IN_DURATION + HOLD_DURATION)
    {
        const float fadeOutElapsed{ elapsedTime - FADE_IN_DURATION - HOLD_DURATION };
        opacity = 1.f - fadeOutElapsed / FADE_OUT_DURATION;
    }

    opacity = std::clamp(opacity, 0.f, 1.f);
    logo.setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(opacity * 255.f)));
}
