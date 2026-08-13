#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include "ui/GlowingCursor.h"
#include "ui/MenuButton.h"
#include "ui/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class AssetStore;
class AudioManager;
class GamepadManager;

namespace sf
{
    class Event;
    class RenderTarget;
    class RenderWindow;
}

class ResultScreen
{
public:
	struct Statistics
	{
		int combatScore{ 0 };
		int armorPercent{ 0 };
		int armorBonus{ 0 };
		unsigned int shotsHit{ 0u };
		unsigned int shotsFired{ 0u };
		int accuracyPercent{ 0 };
		int targetAccuracyPercent{ 0 };
		int accuracyBonus{ 0 };
		float completionSeconds{ 0.f };
		float targetSeconds{ 0.f };
		int timeBonus{ 0 };
		int levelTotal{ 0 };
		int campaignTotal{ 0 };
	};

    enum class Mode
    {
        LevelComplete,
		LevelReplay,
        Victory
    };

    enum class Action
    {
        Primary,
        MainMenu
    };

    ResultScreen(AssetStore& assets, AudioManager& audio, GamepadManager& gamepad,
        sf::Vector2f logicalSize);

    void Start(Mode mode, int level, const Statistics& statistics);
    void Reset();
    void Update(float deltaTime);
    [[nodiscard]] std::optional<Action> HandleEvent(
        const sf::Event& event, sf::RenderWindow& window);
    void Draw(sf::RenderTarget& target);
    void DrawCursor(sf::RenderWindow& window);

    [[nodiscard]] bool IsActive() const noexcept;
    [[nodiscard]] Mode GetMode() const noexcept;

private:
    void SkipAnimation();
    void ApplyVisualState();
    void Select(std::size_t index, bool playSound = true);
    void SelectPrevious();
    void SelectNext();
    void UpdateMouseSelection(sf::Vector2f position);
    [[nodiscard]] std::optional<Action> ActivateSelected();
    void CenterText(sf::Text& text, sf::Vector2f position);

    AudioManager& audio;
    GamepadManager& gamepad;
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
    NeonGlow titleGlow;
    NeonGlow buttonGlow;
    GlowingCursor menuCursor;
    std::vector<MenuButton> buttons;
    std::size_t selectedIndex{ 0u };
    Mode mode{ Mode::LevelComplete };
    float elapsed{ 0.f };
    bool active{ false };
    bool interactive{ false };
};
