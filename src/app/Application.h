#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>

#include "assets/AssetStore.h"
#include "states/StateStack.h"

class Application
{
public:
    Application();

    void Run();

private:
    [[nodiscard]] static sf::View GetLetterboxView(
        const sf::View& view,
        unsigned int windowWidth,
        unsigned int windowHeight);

    static constexpr sf::Vector2f LOGICAL_SIZE{ 1920.f, 1080.f };
    static constexpr float MAX_FRAME_TIME{ 0.1f };

    sf::RenderWindow window;
    AssetStore assets;
    StateStack stateStack;
};
