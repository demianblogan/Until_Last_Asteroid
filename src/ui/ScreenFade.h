#pragma once

#include <SFML/Graphics/RectangleShape.hpp>

namespace sf
{
	class RenderTarget;
}

namespace UI
{
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
		Direction direction = Direction::In;
		float fadeDurationSeconds = 0.f;
		float elapsedSeconds = 0.f;
		bool isActive = false;
	};
}