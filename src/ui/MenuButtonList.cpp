#include "MenuButtonList.h"

#include <utility>

#include "audio/AudioManager.h"
#include "rendering/NeonGlow.h"
#include "utils/ConfigEnums.h"

namespace UI
{
	MenuButtonList::MenuButtonList(AudioManager& audioManager, NeonGlow& glowEffect)
		: audio(audioManager), selectionGlowEffect(glowEffect)
	{}

	void MenuButtonList::Add(MenuButton button)
	{
		buttons.push_back(std::move(button));
	}

	void MenuButtonList::Select(std::size_t index, bool needToPlaySound)
	{
		if (index >= buttons.size() || !buttons[index].IsEnabled())
			return;

		const bool isSelectionChanged = selectedIndex != index;
		selectedIndex = index;

		for (std::size_t buttonIndex = 0u; buttonIndex < buttons.size(); ++buttonIndex)
			buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);

		if (isSelectionChanged)
			selectionGlowEffect.Invalidate();

		if (isSelectionChanged && needToPlaySound)
			audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
	}

	void MenuButtonList::SelectPrevious()
	{
		std::size_t index = selectedIndex;
		do
		{
			index = index == 0u ? buttons.size() - 1u : index - 1u;

		} while (!buttons[index].IsEnabled() && index != selectedIndex);

		Select(index);
	}

	void MenuButtonList::SelectNext()
	{
		std::size_t index = selectedIndex;
		do
		{
			index = (index + 1u) % buttons.size();

		} while (!buttons[index].IsEnabled() && index != selectedIndex);

		Select(index);
	}

	void MenuButtonList::UpdateMouseSelection(sf::Vector2f position)
	{
		for (std::size_t index = 0u; index < buttons.size(); ++index)
		{
			if (buttons[index].Contains(position))
			{
				Select(index);
				return;
			}
		}
	}

	std::size_t MenuButtonList::GetSelectedIndex() const noexcept
	{
		return selectedIndex;
	}

	std::vector<MenuButton>& MenuButtonList::GetButtons() noexcept
	{
		return buttons;
	}

	const std::vector<MenuButton>& MenuButtonList::GetButtons() const noexcept
	{
		return buttons;
	}
}