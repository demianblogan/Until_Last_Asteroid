#pragma once

#include <cstddef>

#include <SFML/Graphics/Shape.hpp>
#include <SFML/System/Vector2.hpp>

class RoundedRectangleShape final : public sf::Shape
{
public:
    RoundedRectangleShape(
        sf::Vector2f size = {},
        float radius = 0.f,
        std::size_t cornerPointCount = 8u);

    void SetSize(sf::Vector2f size);
    void SetRadius(float radius);

    [[nodiscard]] sf::Vector2f GetSize() const noexcept;
    [[nodiscard]] std::size_t getPointCount() const override;
    [[nodiscard]] sf::Vector2f getPoint(std::size_t index) const override;

private:
    sf::Vector2f size;
    float radius{ 0.f };
    std::size_t cornerPointCount{ 8u };
};
