#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/System/Vector2.hpp>

namespace Rendering
{
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

		// For a system that's fully cleared and re-emitted every frame instead
		// of aging (e.g. projectile glows, which just track a moving position
		// with no genuine lifetime) -- rebuilds the vertex data for this
		// frame's freshly spawned particles without applying Update()'s
		// per-frame motion/drag/aging, none of which should happen to a
		// particle that only exists for the one frame it was just spawned in.
		void RefreshVertices();

		void Clear();

		[[nodiscard]] std::size_t GetParticleCount() const noexcept;

	private:
		// A spawned particle's live simulation state: everything ParticleSpawn
		// specified at birth, plus the one field that only makes sense once a
		// particle actually exists -- how much longer it has left to live.
		struct Particle : ParticleSpawn
		{
			float remaining = 1.f;
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
}