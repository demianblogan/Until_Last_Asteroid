#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "ui/GlowingCursor.h"
#include "ui/MenuButtonList.h"
#include "rendering/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

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
	class ResultScreen
	{
	public:
		struct Statistics
		{
			int combatScore = 0;
			int armorPercent = 0;
			int armorBonus = 0;
			unsigned int attacksHit = 0u;
			unsigned int attacksFired = 0u;
			int accuracyPercent = 0;
			int targetAccuracyPercent = 0;
			int accuracyBonus = 0;
			int partsCollected = 0;
			int partsTotal = 0;
			int partsBonus = 0;
			float completionSeconds = 0.f;
			int levelTotal = 0;
		};

		enum class Mode
		{
			LevelComplete,
			LevelReplay,
			ContentComplete,
			Victory
		};

		enum class Action
		{
			Primary,
			Restart,
			MainMenu
		};

		ResultScreen(Assets& assets, AudioManager& audio, GamepadManager& gamepad,
			LocalizationManager& localization,
			sf::Vector2f logicalSize);

		void Start(Mode mode, int level, const Statistics& statistics);
		void Reset();

		void Update(float deltaTime);

		[[nodiscard]] std::optional<Action> HandleEvent(const sf::Event& event, sf::RenderWindow& window);

		void Draw(sf::RenderTarget& target);
		void DrawCursor(sf::RenderWindow& window);

		[[nodiscard]] bool IsActive() const noexcept;
		[[nodiscard]] Mode GetMode() const noexcept;

	private:
		void SkipAnimation();
		void ApplyContent(Mode newMode, int level, const Statistics& resultStatistics);
		void ApplyVisualState();
		[[nodiscard]] std::optional<Action> ActivateSelected();

		Assets& assets;
		AudioManager& audio;
		GamepadManager& gamepad;
		LocalizationManager& localization;

		sf::Vector2f logicalSize;
		sf::RectangleShape shade;
		sf::Sprite titleFrame;
		sf::Text title;
		RoundedRectangleShape statisticsPanel;
		sf::RectangleShape statisticsSeparator;
		sf::Text statisticsTitle;
		std::vector<sf::Text> statisticLabels;
		std::vector<sf::Text> statisticValues;
		Statistics statistics;
		Rendering::NeonGlow titleGlow;
		Rendering::NeonGlow buttonGlow;
		GlowingCursor menuCursor;
		MenuButtonList buttonList;
		Mode mode = Mode::LevelComplete;
		float animationElapsedSeconds = 0.f;
		bool isActive = false;
		bool isInteractive = false;
	};
}