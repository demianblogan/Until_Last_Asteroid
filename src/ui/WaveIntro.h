#pragma once

#include <SFML/Graphics/Text.hpp>

#include "ui/NeonGlow.h"

class AssetStore;

namespace sf { class RenderTarget; }

class WaveIntro
{
public:
    explicit WaveIntro(AssetStore& assets);

    void Start(int waveNumber);
    bool Update(float deltaTime);
    void Draw(sf::RenderTarget& target);

    [[nodiscard]] bool IsActive() const noexcept;

private:
    void ApplyAnimation();
    static void CenterText(sf::Text& text, sf::Vector2f position);

    NeonGlow titleGlow;
    sf::Text title;
    float elapsed{ 0.f };
    bool active{ false };
};
