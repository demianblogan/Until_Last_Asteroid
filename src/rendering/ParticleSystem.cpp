#include "ParticleSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Vertex.hpp>

namespace Rendering
{
	namespace
	{
		sf::Image CreateParticleImage(ParticleAppearance appearance)
		{
			constexpr unsigned int Size = 64;
			constexpr float Center = (static_cast<float>(Size) - 1.f) * 0.5f;
			sf::Image image({ Size, Size }, sf::Color::Transparent);

			for (unsigned int y = 0; y < Size; y++)
			{
				for (unsigned int x = 0; x < Size; x++)
				{
					const float dx = (static_cast<float>(x) - Center) / Center;
					const float dy = (static_cast<float>(y) - Center) / Center;
					const float distance = std::sqrt(dx * dx + dy * dy);
					float intensity = 0.f;

					switch (appearance)
					{
					case ParticleAppearance::Glow:
						intensity = std::pow(std::max(0.f, 1.f - distance), 1.8f);
						break;

					case ParticleAppearance::Smoke:
					{
						const float cloud = std::pow(std::max(0.f, 1.f - distance), 1.35f);
						const float noise = 0.78f + 0.22f * std::sin(static_cast<float>(x) * 0.73f + static_cast<float>(y) * 1.17f);
						intensity = cloud * noise * 0.82f;
						break;
					}

					case ParticleAppearance::Debris:
					{
						const float angle = std::atan2(dy, dx);
						const float edge = 0.55f + 0.1f * std::sin(angle * 5.f) + 0.06f * std::cos(angle * 3.f);
						intensity = std::clamp((edge - distance) * 18.f, 0.f, 1.f);
						break;
					}

					case ParticleAppearance::Ring:
					{
						const float ringDistance = (distance - 0.68f) / 0.075f;
						intensity = std::exp(-ringDistance * ringDistance) * std::clamp((1.f - distance) * 4.f, 0.f, 1.f);
						break;
					}

					}

					image.setPixel(
						{ x, y },
						{ 255, 255, 255, static_cast<std::uint8_t>(std::clamp(intensity * 255.f, 0.f, 255.f)) });
				}
			}

			return image;
		}

		std::uint8_t LerpChannel(std::uint8_t from, std::uint8_t to, float progress)
		{
			return static_cast<std::uint8_t>(std::clamp(
				static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * progress,
				0.f,
				255.f));
		}

		sf::Color LerpColor(const sf::Color& from, const sf::Color& to, float progress)
		{
			return
			{
				LerpChannel(from.r, to.r, progress),
				LerpChannel(from.g, to.g, progress),
				LerpChannel(from.b, to.b, progress),
				LerpChannel(from.a, to.a, progress)
			};
		}
	}

	ParticleSystem::ParticleSystem(ParticleAppearance appearance)
		: glowTexture(CreateParticleImage(appearance))
		, isAdditiveBlend(appearance == ParticleAppearance::Glow || appearance == ParticleAppearance::Ring)
	{
		glowTexture.setSmooth(true);
		particles.reserve(MaximumParticles);
	}

	void ParticleSystem::Emit(const ParticleSpawn& spawn)
	{
		if (spawn.lifetime <= 0.f || particles.size() >= MaximumParticles)
			return;

		particles.push_back({ spawn, spawn.lifetime });
	}

	void ParticleSystem::Update(float deltaTime)
	{
		for (std::size_t i = 0; i < particles.size();)
		{
			Particle& particle = particles[i];

			particle.remaining -= deltaTime;

			if (particle.remaining <= 0.f)
			{
				particle = particles.back();
				particles.pop_back();
				continue;
			}

			particle.position += particle.velocity * deltaTime;
			particle.rotation += particle.angularVelocity * deltaTime;
			particle.velocity *= std::exp(-particle.drag * deltaTime);

			i++;
		}

		BuildVertices();
	}

	void ParticleSystem::RefreshVertices()
	{
		BuildVertices();
	}

	void ParticleSystem::Clear()
	{
		particles.clear();
		vertices.clear();
	}

	std::size_t ParticleSystem::GetParticleCount() const noexcept
	{
		return particles.size();
	}

	void ParticleSystem::BuildVertices()
	{
		vertices.clear();

		for (const Particle& particle : particles)
		{
			const float progress = std::clamp(1.f - particle.remaining / particle.lifetime, 0.f, 1.f);
			AppendParticleQuad(particle, progress);
		}
	}

	void ParticleSystem::AppendParticleQuad(const Particle& particle, float progress)
	{
		const float size = particle.startSize + (particle.endSize - particle.startSize) * progress;
		const float halfWidth = size * particle.aspectRatio * 0.5f;
		const float halfHeight = size * 0.5f;
		const float cosine = std::cos(particle.rotation);
		const float sine = std::sin(particle.rotation);
		const sf::Color color{ LerpColor(particle.startColor, particle.endColor, progress) };

		const auto transformPoint = [&](sf::Vector2f point)
			{
				return particle.position + sf::Vector2f{
					point.x * cosine - point.y * sine,
					point.x * sine + point.y * cosine };
			};

		const sf::Vector2f topLeft{ transformPoint({ -halfWidth, -halfHeight }) };
		const sf::Vector2f topRight{ transformPoint({ halfWidth, -halfHeight }) };
		const sf::Vector2f bottomLeft{ transformPoint({ -halfWidth, halfHeight }) };
		const sf::Vector2f bottomRight{ transformPoint({ halfWidth, halfHeight }) };

		vertices.append({ topLeft, color, { 0.f, 0.f } });
		vertices.append({ bottomLeft, color, { 0.f, TextureSize } });
		vertices.append({ bottomRight, color, { TextureSize, TextureSize } });
		vertices.append({ topLeft, color, { 0.f, 0.f } });
		vertices.append({ bottomRight, color, { TextureSize, TextureSize } });
		vertices.append({ topRight, color, { TextureSize, 0.f } });
	}

	void ParticleSystem::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		states.blendMode = isAdditiveBlend ? sf::BlendAdd : sf::BlendAlpha;
		states.texture = &glowTexture;

		target.draw(vertices, states);
	}
}