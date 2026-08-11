#pragma once

#include <SFML/Graphics/RectangleShape.hpp>

namespace sf { class RenderTarget; }

class ScreenFade
{
public:
    explicit ScreenFade(sf::Vector2f size);

    void StartFadeIn(float duration);
    void StartFadeOut(float duration);
    void Update(float deltaTime);
    void Draw(sf::RenderTarget& target) const;

    [[nodiscard]] bool IsActive() const noexcept;

private:
    enum class Direction
    {
        In,
        Out
    };

    void Start(Direction direction, float duration);
    void ApplyOpacity(float progress);

    sf::RectangleShape overlay;
    Direction direction{ Direction::In };
    float duration{ 0.f };
    float elapsed{ 0.f };
    bool active{ false };
};
