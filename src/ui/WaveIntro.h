#pragma once

#include <SFML/Graphics/Text.hpp>

#include "ui/NeonGlow.h"

class AssetStore;
class AudioManager;

namespace sf { class RenderTarget; }

class WaveIntro
{
public:
    WaveIntro(AssetStore& assets, AudioManager& audio);

    void Start(int waveNumber);
    bool Update(float deltaTime);
    void Draw(sf::RenderTarget& target);

    [[nodiscard]] bool IsActive() const noexcept;

private:
    void ApplyAnimation();
    static void CenterText(sf::Text& text, sf::Vector2f position);

    NeonGlow titleGlow;
    NeonGlow countdownGlow;
    sf::Text title;
    sf::Text countdown;
    AudioManager& audio;
	float countdownStepDuration{ 0.f };
	float totalDuration{ 0.f };
    float elapsed{ 0.f };
	bool countdownSoundStarted{ false };
    bool active{ false };
};
