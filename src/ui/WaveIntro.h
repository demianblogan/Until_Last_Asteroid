#pragma once

#include <SFML/Graphics/Text.hpp>

#include "ui/NeonGlow.h"

class Assets;
class LocalizationManager;

namespace sf { class RenderTarget; }

class WaveIntro
{
public:
    WaveIntro(Assets& assets, LocalizationManager& localization);

    void Start(int waveNumber, bool finalWave = false);
    bool Update(float deltaTime);
    void Draw(sf::RenderTarget& target);
    void Reset() noexcept;

    [[nodiscard]] bool IsActive() const noexcept;

private:
    void ApplyAnimation();
    static void CenterText(sf::Text& text, sf::Vector2f position);

    NeonGlow titleGlow;
    Assets& assets;
    LocalizationManager& localization;
    sf::Text title;
    float elapsed{ 0.f };
    bool active{ false };
};
