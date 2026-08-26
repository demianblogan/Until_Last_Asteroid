#pragma once

#include <SFML/Graphics/Text.hpp>

#include "rendering/NeonGlow.h"

class Assets;
class LocalizationManager;

namespace sf
{
	class RenderTarget;
}

namespace UI
{
	class WaveIntro
	{
	public:
		WaveIntro(Assets& assets, LocalizationManager& localization);

		void Start(int waveNumber, bool isFinalWave = false);
		void Reset() noexcept;

		bool Update(float deltaTime);
		void Draw(sf::RenderTarget& target);

		[[nodiscard]] bool IsActive() const noexcept;

	private:
		void ApplyAnimation();

		Rendering::NeonGlow titleGlow;
		Assets& assets;
		LocalizationManager& localization;
		sf::Text title;
		float elapsedSeconds = 0.f;
		bool isActive = false;
	};
}