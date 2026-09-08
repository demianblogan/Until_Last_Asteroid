#pragma once

#include <cstddef>
#include <vector>

#include <SFML/System/Vector2.hpp>

#include "ui/MenuButton.h"

class AudioManager;

namespace Rendering
{
	class NeonGlow;
}

namespace UI
{
	// A row/column of MenuButtons with the keyboard/gamepad/mouse selection
	// logic shared by every screen that has one (GameOverScreen, ResultScreen,
	// MainMenuState, PauseState, LevelSelectState, CampaignMenuState, ...):
	// wrap-around Previous/Next that skips disabled buttons, click-to-select,
	// and invalidating the selection glow + playing a sound exactly when the
	// selected button changes.
	class MenuButtonList
	{
	public:
		MenuButtonList(AudioManager& audio, Rendering::NeonGlow& selectionGlowEffect);

		void Add(MenuButton button);

		void Select(std::size_t index, bool needToPlaySound = true);
		void SelectPrevious();
		void SelectNext();
		void UpdateMouseSelection(sf::Vector2f position);

		[[nodiscard]] std::size_t GetSelectedIndex() const noexcept;
		[[nodiscard]] std::vector<MenuButton>& GetButtons() noexcept;
		[[nodiscard]] const std::vector<MenuButton>& GetButtons() const noexcept;

	private:
		AudioManager& audio;
		Rendering::NeonGlow& selectionGlowEffect;
		std::vector<MenuButton> buttons;
		std::size_t selectedIndex = 0u;
	};
}