#include "CompanySplashState.h"

#include <algorithm>
#include <cstdint>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "states/StateId.h"
#include "utils/ConfigEnums.h"

CompanySplashState::CompanySplashState(StateStack& stateStack, StateContext context)
    : State(stateStack, context)
    , logo(context.assets.Textures().Get(Config::Texture::CompanyLogo))
{
    context.window.setMouseCursorVisible(false);

    const sf::Vector2u textureSize{ logo.getTexture().getSize() };
    logo.setOrigin({
        static_cast<float>(textureSize.x) * 0.5f,
        static_cast<float>(textureSize.y) * 0.5f
    });
    UpdateLayout();
    logo.setColor(sf::Color(255, 255, 255, 0));

    context.audio.PlayMusic(Config::Music::CompanySplash, false);
}

CompanySplashState::~CompanySplashState()
{
    GetContext().audio.StopMusic(Config::Music::CompanySplash);
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
    sf::RenderWindow& window{ GetContext().window };
    const sf::View previousView{ window.getView() };
    window.setView(window.getDefaultView());
    UpdateLayout();
    window.draw(logo);
    window.setView(previousView);
}

void CompanySplashState::UpdateLayout()
{
    const sf::Vector2u windowSize{ GetContext().window.getSize() };
    const sf::Vector2u textureSize{ logo.getTexture().getSize() };
    if (windowSize.x == 0u || windowSize.y == 0u ||
        textureSize.x == 0u || textureSize.y == 0u)
        return;

    logo.setScale({
        static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x),
        static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y)
    });
    logo.setPosition({
        static_cast<float>(windowSize.x) * 0.5f,
        static_cast<float>(windowSize.y) * 0.5f
    });
}

bool CompanySplashState::IsSkipEvent(const sf::Event& event)
{
    return event.is<sf::Event::KeyPressed>() ||
        event.is<sf::Event::MouseButtonPressed>() ||
        event.is<sf::Event::JoystickButtonPressed>();
}

void CompanySplashState::Finish()
{
    if (isFinishing)
        return;

    isFinishing = true;
    GetContext().audio.StopMusic(Config::Music::CompanySplash);
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
