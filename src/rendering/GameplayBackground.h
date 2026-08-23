#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

class Assets;

class GameplayBackground final : public sf::Drawable
{
public:
    GameplayBackground(Assets& assets, sf::Vector2f logicalSize);

    void SetTheme(std::string_view theme, float brightness = 1.f);
    void Update(float deltaTime);

private:
    struct Star
    {
        sf::Vector2f position;
        float size{ 1.f };
        float speed{ 0.f };
        sf::Color color{ sf::Color::White };
    };

    void BuildGradient();
    void GenerateStars(std::uint32_t seed);
    void UpdateLayer(std::vector<Star>& layer, float deltaTime);
    void AppendStarQuad(sf::VertexArray& vertices, const Star& star) const;
    void BuildStarVertices(const std::vector<Star>& layer, sf::VertexArray& vertices) const;
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    Assets& assets;
    sf::Vector2f logicalSize;
    sf::Color topColor;
    sf::Color bottomColor;
	sf::Color starColor{ 185, 220, 255 };
    sf::VertexArray gradient{ sf::PrimitiveType::Triangles };
    sf::VertexArray middleStarVertices{ sf::PrimitiveType::Triangles };
    sf::VertexArray nearDustVertices{ sf::PrimitiveType::Triangles };
    std::optional<sf::Sprite> farBackground;
    std::vector<Star> middleStars;
    std::vector<Star> nearDust;
};
