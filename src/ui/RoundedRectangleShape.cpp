#include "RoundedRectangleShape.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

RoundedRectangleShape::RoundedRectangleShape(
    sf::Vector2f shapeSize,
    float cornerRadius,
    std::size_t pointsPerCorner)
    : size(shapeSize)
    , radius(cornerRadius)
    , cornerPointCount(std::max<std::size_t>(2u, pointsPerCorner))
{
    update();
}

void RoundedRectangleShape::SetSize(sf::Vector2f shapeSize)
{
    size = shapeSize;
    update();
}

void RoundedRectangleShape::SetRadius(float cornerRadius)
{
    radius = cornerRadius;
    update();
}

sf::Vector2f RoundedRectangleShape::GetSize() const noexcept
{
    return size;
}

std::size_t RoundedRectangleShape::getPointCount() const
{
    return cornerPointCount * 4u;
}

sf::Vector2f RoundedRectangleShape::getPoint(std::size_t index) const
{
    const float clampedRadius{ std::clamp(radius, 0.f, std::min(size.x, size.y) * 0.5f) };
    const std::size_t corner{ index / cornerPointCount };
    const std::size_t pointInCorner{ index % cornerPointCount };
    const float progress{ static_cast<float>(pointInCorner) /
        static_cast<float>(cornerPointCount - 1u) };
    const float halfPi{ std::numbers::pi_v<float> * 0.5f };
    const float angle{ (static_cast<float>(corner) * halfPi) +
        std::numbers::pi_v<float> + progress * halfPi };

    const std::array<sf::Vector2f, 4> centers{
        sf::Vector2f{ clampedRadius, clampedRadius },
        sf::Vector2f{ size.x - clampedRadius, clampedRadius },
        sf::Vector2f{ size.x - clampedRadius, size.y - clampedRadius },
        sf::Vector2f{ clampedRadius, size.y - clampedRadius }
    };

    return centers[corner] + sf::Vector2f{ std::cos(angle), std::sin(angle) } * clampedRadius;
}
