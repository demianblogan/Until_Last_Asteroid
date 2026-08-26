#include "LevelSelectState.h"

#include <algorithm>
#include <string>
#include <utility>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "campaign/CampaignSaveManager.h"
#include "gameplay/GameplayData.h"
#include "gameplay/GameplayLaunch.h"
#include "localization/LocalizationManager.h"
#include "states/StateID.h"
#include "input/GamepadManager.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr int VisibleLevelCount{ 10 };
	constexpr sf::Vector2f LevelButtonSize{ 650.f, 72.f };
	constexpr sf::Vector2f PartsFrameSize{ 200.f, 72.f };
	constexpr sf::Vector2f ReturnButtonSize{ 620.f, 72.f };
	constexpr sf::Vector2f FirstButtonPosition{ 525.f, 125.f };
	constexpr float PartsFrameGap{ 20.f };
	constexpr float ButtonSpacing{ 75.f };
	constexpr sf::Color Cyan{ 25, 220, 255 };
	constexpr sf::Color Amber{ 255, 178, 42 };
	constexpr float FadeDuration{ 0.38f };
}

LevelSelectState::LevelSelectState(StateStack& stateStack, StateContext context)
	: State(stateStack, context)
	, background(context.assets, context.logicalSize)
	, titleGlow(context.assets)
	, buttonGlow(context.assets)
	, partsGlow(context.assets)
	, menuCursor(context.assets, Config::Texture::MenuPointer, { 6.f, 2.f }, Cyan)
	, screenFade(context.logicalSize)
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("level_select.title"), 72u)
	, buttonList(context.audio, buttonGlow)
{
	context.window.setMouseCursorVisible(false);
	title.setFillColor(sf::Color(215, 247, 252));
	title.setOutlineColor(sf::Color(3, 18, 31, 235));
	title.setOutlineThickness(3.5f);
	title.setLetterSpacing(1.12f);
	UI::TextLayout::CenterText(title, { context.logicalSize.x * 0.5f, 82.f });

	const GameplayData& gameplayData{ context.assets.GetGameplayData() };
	const CampaignProgress* progress{ context.campaignSave.GetProgress() };
	const int highestUnlocked{ progress != nullptr
		? std::max(1, progress->highestUnlockedLevel)
		: 1 };
	const int implementedLevels{ gameplayData.GetLevelCount() };
	const sf::Font& menuFont{ context.assets.Fonts().Get(context.localization.GetRegularFont()) };
	const sf::Texture& idle{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
	const sf::Texture& selected{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };

	partsFrames.reserve(VisibleLevelCount);
	partsIcons.reserve(VisibleLevelCount);
	partsCounts.reserve(VisibleLevelCount);
	buttonLevels.reserve(VisibleLevelCount + 1u);
	const sf::Texture& partTexture{
		context.assets.Textures().Get(Config::Texture::PartToken) };
	const sf::Vector2u partTextureSize{ partTexture.getSize() };
	const float partIconScale{ 44.f / static_cast<float>(
		std::max(partTextureSize.x, partTextureSize.y)) };
	for (int level{ 1 }; level <= VisibleLevelCount; ++level)
	{
		const bool implemented{ level <= implementedLevels };
		const bool enabled{ implemented && level <= highestUnlocked };
		sf::String label{ std::to_string(level) + "  -  " };
		label += context.localization.GetText("level_select.locked");
		int collected{ 0 };
		std::size_t total{ 0u };
		if (implemented)
		{
			const auto& levelConfig{ gameplayData.GetLevel(level) };
			total = levelConfig.partIds.size();
			collected = progress == nullptr ? 0 : static_cast<int>(
				std::ranges::count_if(levelConfig.partIds, [progress](const std::string& id)
				{
					return std::ranges::find(progress->collectedPartIDs, id) !=
						progress->collectedPartIDs.end();
				}));
			if (enabled)
			{
				label = sf::String(std::to_string(level) + "  -  ");
				label += context.localization.GetText(
					"levels.title_" + std::to_string(level));
			}
		}
		const sf::Vector2f rowPosition{ FirstButtonPosition +
			sf::Vector2f{ 0.f, ButtonSpacing * static_cast<float>(level - 1) } };
		UI::MenuButton button(menuFont, idle, selected, "", LevelButtonSize);
		button.SetLabel(label);
		button.SetPosition(rowPosition);
		button.SetEnabled(enabled);
		buttonList.Add(std::move(button));

		const sf::Vector2f partsPosition{
			rowPosition.x + LevelButtonSize.x + PartsFrameGap, rowPosition.y };
		partsFrames.emplace_back(menuFont, idle, selected, "", PartsFrameSize);
		partsFrames.back().SetPosition(partsPosition);
		partsFrames.back().SetEnabled(enabled);
		partsIcons.emplace_back(partTexture);
		partsIcons.back().setOrigin({
			static_cast<float>(partTextureSize.x) * 0.5f,
			static_cast<float>(partTextureSize.y) * 0.5f });
		partsIcons.back().setScale({ partIconScale, partIconScale });
		partsIcons.back().setPosition(partsPosition + sf::Vector2f{ 43.f, 36.f });
		partsIcons.back().setColor(enabled
			? sf::Color::White
			: sf::Color(90, 90, 90));
		partsCounts.emplace_back(menuFont,
			implemented
				? std::to_string(collected) + " / " + std::to_string(total)
				: "- / -",
			30u);
		partsCounts.back().setFillColor(enabled
			? sf::Color(225, 245, 250)
			: sf::Color(100, 112, 122));
		partsCounts.back().setOutlineColor(sf::Color(3, 18, 31, 230));
		partsCounts.back().setOutlineThickness(2.f);
		UI::TextLayout::CenterText(partsCounts.back(), partsPosition + sf::Vector2f{ 128.f, 36.f });
		buttonLevels.push_back(level);
	}

	UI::MenuButton returnButton(
		menuFont, idle, selected, "", ReturnButtonSize);
	returnButton.SetLabel(context.localization.GetText("level_select.back_game_menu"));
	returnButton.SetPosition({
		(context.logicalSize.x - ReturnButtonSize.x) * 0.5f, 900.f });
	buttonList.Add(std::move(returnButton));
	buttonLevels.push_back(0);
	Select(0u, false);
	screenFade.StartFadeIn(0.25f);
}

void LevelSelectState::HandleEvent(const sf::Event& event)
{
	if (launchingLevel || screenFade.IsActive())
		return;

	using enum GamepadManager::NavigationAction;
	switch (GetContext().gamepad.GetNavigationAction(event))
	{
	case Up: SelectPrevious(); return;
	case Down: SelectNext(); return;
	case Confirm: ActivateSelected(); return;
	case Back: RequestPop(); return;
	default: break;
	}

	if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
		background.SetMousePosition(point);
		UpdateMouseSelection(point);
		return;
	}
	if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() })
	{
		if (pressed->button != sf::Mouse::Button::Left)
			return;
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pressed->position) };
		for (std::size_t index{ 0u }; index < buttonList.GetButtons().size(); ++index)
		{
			const bool containsParts{ index < partsFrames.size() &&
				partsFrames[index].Contains(point) };
			if (buttonList.GetButtons()[index].IsEnabled() &&
				(buttonList.GetButtons()[index].Contains(point) || containsParts))
			{
				Select(index, false);
				ActivateSelected();
				return;
			}
		}
	}
	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		switch (key->code)
		{
		case sf::Keyboard::Key::Up:
		case sf::Keyboard::Key::W: SelectPrevious(); break;
		case sf::Keyboard::Key::Down:
		case sf::Keyboard::Key::S: SelectNext(); break;
		case sf::Keyboard::Key::Enter:
		case sf::Keyboard::Key::Space: ActivateSelected(); break;
		case sf::Keyboard::Key::Escape: RequestPop(); break;
		default: break;
		}
	}
}

void LevelSelectState::Update(float deltaTime)
{
	background.Update(deltaTime);
	titleGlow.Update(deltaTime);
	buttonGlow.Update(deltaTime);
	partsGlow.Update(deltaTime);
	menuCursor.Update(deltaTime);
	screenFade.Update(deltaTime);
	if (launchingLevel && !screenFade.IsActive())
	{
		RequestClear();
		RequestPush(StateID::Gameplay);
	}
}

void LevelSelectState::Render()
{
	sf::RenderWindow& window{ GetContext().window };
	background.Draw(window);
	titleGlow.DrawBloom(window, title.getGlobalBounds(),
		[this](sf::RenderTarget& target, const sf::RenderStates& states)
		{ target.draw(title, states); }, Cyan);
	window.draw(title);
	titleGlow.DrawHighlight(window, title.getGlobalBounds(), Cyan);

	const std::size_t selectedIndex{ buttonList.GetSelectedIndex() };
	const UI::MenuButton& selectedButton{ buttonList.GetButtons()[selectedIndex] };
	buttonGlow.DrawBloom(window, selectedButton.GetBounds(),
		[&selectedButton](sf::RenderTarget& target, const sf::RenderStates& states)
		{ selectedButton.Draw(target, states); }, Amber);
	if (selectedIndex < partsFrames.size())
	{
		const std::size_t index{ selectedIndex };
		partsGlow.DrawBloom(window, partsFrames[index].GetBounds(),
			[this, index](sf::RenderTarget& target, const sf::RenderStates& states)
			{
				partsFrames[index].Draw(target, states);
				target.draw(partsIcons[index], states);
				target.draw(partsCounts[index], states);
			}, Amber);
	}
	for (const UI::MenuButton& button : buttonList.GetButtons())
		button.Draw(window);
	for (std::size_t index{ 0u }; index < partsFrames.size(); ++index)
	{
		partsFrames[index].Draw(window);
		window.draw(partsIcons[index]);
		window.draw(partsCounts[index]);
	}
	buttonGlow.DrawHighlight(window, selectedButton.GetBounds(), Amber);
	if (selectedIndex < partsFrames.size())
		partsGlow.DrawHighlight(
			window, partsFrames[selectedIndex].GetBounds(), Amber);
}

void LevelSelectState::RenderOverlay()
{
	if (!GetContext().gamepad.IsInUse())
		menuCursor.Draw(GetContext().window);
	screenFade.Draw(GetContext().window);
}

void LevelSelectState::SelectPrevious()
{
	buttonList.SelectPrevious();
	SyncPartsSelection();
}

void LevelSelectState::SelectNext()
{
	buttonList.SelectNext();
	SyncPartsSelection();
}

void LevelSelectState::Select(std::size_t index, bool playSound)
{
	buttonList.Select(index, playSound);
	SyncPartsSelection();
}

void LevelSelectState::UpdateMouseSelection(sf::Vector2f position)
{
	for (std::size_t index{ 0u }; index < buttonList.GetButtons().size(); ++index)
	{
		const bool containsParts{ index < partsFrames.size() &&
			partsFrames[index].Contains(position) };
		if (buttonList.GetButtons()[index].IsEnabled() &&
			(buttonList.GetButtons()[index].Contains(position) || containsParts))
		{
			Select(index);
			return;
		}
	}
}

void LevelSelectState::SyncPartsSelection()
{
	const std::size_t selectedIndex{ buttonList.GetSelectedIndex() };
	for (std::size_t frameIndex{ 0u }; frameIndex < partsFrames.size(); ++frameIndex)
		partsFrames[frameIndex].SetSelected(frameIndex == selectedIndex);
	partsGlow.Invalidate();
}

void LevelSelectState::ActivateSelected()
{
	GetContext().audio.PlaySound(
		Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f,
		SoundPlayback::StopPrevious);
	const int level{ buttonLevels[buttonList.GetSelectedIndex()] };
	if (level == 0)
	{
		RequestPop();
		return;
	}
	BeginLevel(level);
}

void LevelSelectState::BeginLevel(int level)
{
	GetContext().gameplayLaunch.mode = GameplayLaunchMode::SelectedLevel;
	GetContext().gameplayLaunch.selectedLevel = level;
	launchingLevel = true;
	screenFade.StartFadeOut(FadeDuration);
}
