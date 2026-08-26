#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "ui/GlowingCursor.h"
#include "ui/LocalizationRevision.h"
#include "ui/MenuButtonList.h"
#include "rendering/NeonGlow.h"

class Assets;
class AudioManager;
class GamepadManager;
class LocalizationManager;

namespace sf
{
	class Event;
	class RenderTarget;
	class RenderWindow;
}

namespace UI
{
	class GameOverScreen
	{
	public:
		enum class Action
		{
			RestartLevel,
			MainMenu
		};

		GameOverScreen(Assets& assets, AudioManager& audio, GamepadManager& gamepad,
			LocalizationManager& localization, sf::Vector2f logicalSize);

		void ShowInCampaignMode(int finalScore);
		void ShowInHordeMode(int finalScore, int wavesSurvived);
		void ShowInRunMode(int survivalSeconds, int recordSeconds);

		void Reset();
		void Update(float deltaTime);
		[[nodiscard]] std::optional<Action> HandleEvent(const sf::Event& event, sf::RenderWindow& window);

		void Draw(sf::RenderTarget& target);
		void DrawCursor(sf::RenderWindow& window);

		[[nodiscard]] bool IsActive() const noexcept;

	private:
		void ShowWithSummary(sf::String summary, const sf::String& restartLabel);
		void RefreshLocalizedContent();
		void SkipAnimation();
		void ApplyVisualState();

		[[nodiscard]] std::optional<Action> ActivateSelectedButton();

		Assets& assets;
		AudioManager& audio;
		GamepadManager& gamepad;
		LocalizationManager& localization;

		sf::Vector2f logicalSize;
		sf::RectangleShape shade;
		sf::Sprite titleFrame;
		sf::Text title;
		sf::Text finalScore;
		NeonGlow titleGlowEffect;
		NeonGlow buttonGlowEffect;

		GlowingCursor menuCursor;
		MenuButtonList buttonList;
		LocalizationRevision localizationRevision;

		float animationElapsedSeconds = 0.f;
		bool isBeingShown = false;
		bool isInteractive = false;
	};
}