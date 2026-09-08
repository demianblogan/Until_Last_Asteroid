#pragma once

#include <cstddef>
#include <vector>

#include <SFML/System/String.hpp>

namespace UI
{
	// Plays the main menu's one-time entrance animation:
	//   1. TypingTitle       -- the title is typed out one character at a time.
	//   2. MovingTitle       -- once fully typed, the title eases from its
	//                           centered "splash" position up to its final
	//                           resting spot at the top of the screen.
	//   3. TypingMenuItems   -- the menu button labels are typed out one at a
	//                           time, in order, each after the previous one.
	//   4. RevealingFrames   -- the button frames fade in from transparent.
	//   5. Interactive       -- the animation is done; the menu now responds
	//                           to input. This phase is permanent -- once
	//                           reached (naturally or via Skip()), the
	//                           animation never restarts.
	// Update() advances whichever phase is currently active and reports, via
	// the returned Events, anything that happened this frame that the caller
	// needs to react to (play a typing sound, start music, etc.). Skip()
	// jumps straight to the Interactive phase, as if the animation had
	// already finished.
	class MenuIntroAnimation
	{
	public:
		struct Events
		{
			std::size_t typedCharacters = 0;
			bool hasActivationStarted = false;
			bool hasBecomeInteractive = false;
		};

		MenuIntroAnimation(sf::String title, std::vector<sf::String> menuItems);

		[[nodiscard]] Events Update(float deltaTime);
		[[nodiscard]] Events Skip();

		[[nodiscard]] sf::String GetVisibleTitle() const;
		[[nodiscard]] sf::String GetVisibleMenuItem(std::size_t index) const;
		[[nodiscard]] float GetTitleMoveProgress() const noexcept;
		[[nodiscard]] float GetFrameOpacity() const noexcept;

		[[nodiscard]] bool IsInteractive() const noexcept;

	private:
		enum class Phase
		{
			TypingTitle,
			MovingTitle,
			TypingMenuItems,
			RevealingFrames,
			Interactive
		};

		void UpdateTypingTitle(float deltaTime, Events& events);
		void UpdateMovingTitle(float deltaTime);
		void UpdateTypingMenuItems(float deltaTime, Events& events);

		void StartFrameReveal(Events& events);

		static constexpr float TitleCharacterInterval = 0.045f;
		static constexpr float TitleMoveDuration = 0.65f;
		static constexpr float MenuCharacterInterval = 0.035f;
		static constexpr float MenuItemPause = 0.06f;
		static constexpr float FrameRevealDuration = 0.28f;

		sf::String title;
		std::vector<sf::String> menuItems;
		std::vector<std::size_t> visibleMenuCharacters;

		Phase phase = Phase::TypingTitle;
		std::size_t visibleTitleCharacters = 0;
		std::size_t currentMenuItem = 0;
		float characterTimer = 0.f;
		float phaseTimer = 0.f;
		float titleMoveProgress = 0.f;
		float frameOpacity = 0.f;
		bool hasStartedActivation = false;
	};
}