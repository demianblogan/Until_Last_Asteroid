#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

struct ParticleSpawn
{
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifetime = 1.f;
    float startSize = 1.f;
    float endSize = 1.f;
    sf::Color startColor{ sf::Color::White };
    sf::Color endColor{ sf::Color::Transparent };
    float rotation = 0.f;
    float angularVelocity = 0.f;
    float drag = 0.f;
    float aspectRatio = 1.f;
};

enum class ParticleAppearance
{
    Glow,
    Smoke,
    Debris,
    Ring
};

class ParticleSystem final : public sf::Drawable
{
public:
    explicit ParticleSystem(ParticleAppearance appearance = ParticleAppearance::Glow);

    void Emit(const ParticleSpawn& spawn);
    void Update(float deltaTime);
    void Clear();

    [[nodiscard]] std::size_t GetParticleCount() const noexcept;

private:
    struct Particle
    {
        sf::Vector2f position;
        sf::Vector2f velocity;
        float lifetime = 1.f;
        float remaining = 1.f;
        float startSize = 1.f;
        float endSize = 1.f;
        sf::Color startColor{ sf::Color::White };
        sf::Color endColor{ sf::Color::Transparent };
        float rotation = 0.f;
        float angularVelocity = 0.f;
        float drag = 0.f;
        float aspectRatio = 1.f;
    };

    void BuildVertices();
    void AppendParticleQuad(const Particle& particle, float progress);
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

    static constexpr std::size_t MaximumParticles = 4096;
    static constexpr float TextureSize = 64.f;

    sf::Texture glowTexture;
    sf::VertexArray vertices{ sf::PrimitiveType::Triangles };
    std::vector<Particle> particles;
    bool isAdditiveBlend = true;
};
