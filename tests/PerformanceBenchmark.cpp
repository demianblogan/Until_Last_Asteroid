#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <numbers>

#include <SFML/Graphics/RenderTexture.hpp>

#include "rendering/ParticleSystem.h"
#include "systems/Collision.h"

namespace
{
    struct SimulatedEntity
    {
        sf::Vector2f position;
        float angle{ 0.f };
        bool compound{ false };
    };

    constexpr std::array SingleCircle{
        Collision::LocalCircle{ {}, 24.f }
    };
    constexpr std::array CompoundCircles{
        Collision::LocalCircle{ { -42.f, 0.f }, 25.f },
        Collision::LocalCircle{ {}, 30.f },
        Collision::LocalCircle{ { 42.f, 0.f }, 25.f }
    };

    sf::Vector2f TransformOffset(const SimulatedEntity& entity, sf::Vector2f offset)
    {
        const float cosine{ std::cos(entity.angle) };
        const float sine{ std::sin(entity.angle) };
        return entity.position + sf::Vector2f{
            offset.x * cosine - offset.y * sine,
            offset.x * sine + offset.y * cosine };
    }

    bool Intersects(const SimulatedEntity& first, const SimulatedEntity& second)
    {
        const auto check{ [&](const auto& firstCircles, const auto& secondCircles)
        {
            for (const Collision::LocalCircle& firstCircle : firstCircles)
            {
                for (const Collision::LocalCircle& secondCircle : secondCircles)
                {
                    if (Collision::Circle(
                        TransformOffset(first, firstCircle.offset), firstCircle.radius,
                        TransformOffset(second, secondCircle.offset), secondCircle.radius))
                    {
                        return true;
                    }
                }
            }
            return false;
        } };

        if (first.compound && second.compound)
            return check(CompoundCircles, CompoundCircles);
        if (first.compound)
            return check(CompoundCircles, SingleCircle);
        if (second.compound)
            return check(SingleCircle, CompoundCircles);
        return check(SingleCircle, SingleCircle);
    }

    double BenchmarkCollisions()
    {
        constexpr std::size_t EntityCount{ 100 };
        constexpr int FrameCount{ 2000 };
        std::array<SimulatedEntity, EntityCount> entities;
        for (std::size_t index{ 0 }; index < entities.size(); ++index)
        {
            entities[index].position = {
                45.f + static_cast<float>(index % 10) * 185.f,
                45.f + static_cast<float>(index / 10) * 105.f };
            entities[index].angle = static_cast<float>(index) * 0.17f;
            entities[index].compound = index % 5 == 0;
        }

        std::size_t collisionCount{ 0 };
        const auto start{ std::chrono::steady_clock::now() };
        for (int frame{ 0 }; frame < FrameCount; ++frame)
        {
            for (SimulatedEntity& entity : entities)
                entity.angle += 0.0025f;

            for (std::size_t first{ 0 }; first < entities.size(); ++first)
            {
                for (std::size_t second{ first + 1 }; second < entities.size(); ++second)
                {
                    if (Intersects(entities[first], entities[second]))
                        ++collisionCount;
                }
            }
        }
        const auto elapsed{ std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count() };
        std::cout << "collision_hits=" << collisionCount << '\n';
        return elapsed / static_cast<double>(FrameCount);
    }

    double BenchmarkParticles()
    {
        constexpr int ParticleCount{ 1000 };
        constexpr int FrameCount{ 360 };
        sf::RenderTexture target({ 1920u, 1080u });
        ParticleSystem glowA;
        ParticleSystem glowB;
        ParticleSystem glowC;
        ParticleSystem smoke(ParticleAppearance::Smoke);
        ParticleSystem debris(ParticleAppearance::Debris);
        ParticleSystem ring(ParticleAppearance::Ring);
        ParticleSystem glowD;
        const std::array systems{
            &glowA, &glowB, &glowC, &smoke, &debris, &ring, &glowD
        };

        for (int index{ 0 }; index < ParticleCount; ++index)
        {
            const float angle{ static_cast<float>(index) * 0.37f };
            systems[static_cast<std::size_t>(index) % systems.size()]->Emit({
                { 960.f + std::cos(angle) * static_cast<float>(index % 700),
                  540.f + std::sin(angle) * static_cast<float>(index % 430) },
                { std::cos(angle) * 18.f, std::sin(angle) * 18.f },
                30.f,
                4.f + static_cast<float>(index % 24),
                2.f,
                { 180, 225, 255, 210 },
                { 30, 70, 110, 0 },
                angle,
                0.25f,
                0.2f,
                index % 3 == 0 ? 2.5f : 1.f });
        }

        const auto start{ std::chrono::steady_clock::now() };
        for (int frame{ 0 }; frame < FrameCount; ++frame)
        {
            target.clear();
            for (ParticleSystem* system : systems)
            {
                system->Update(1.f / 240.f);
                target.draw(*system);
            }
            target.display();
        }
        const auto elapsed{ std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count() };
        return elapsed / static_cast<double>(FrameCount);
    }
}

int main()
{
    const double collisionMilliseconds{ BenchmarkCollisions() };
    const double particleMilliseconds{ BenchmarkParticles() };
    std::cout << std::fixed << std::setprecision(3)
        << "collision_ms_per_frame=" << collisionMilliseconds << '\n'
        << "particles_ms_per_frame=" << particleMilliseconds << '\n'
        << "combined_ms_per_frame=" << collisionMilliseconds + particleMilliseconds << '\n';
}
