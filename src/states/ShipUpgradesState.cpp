#include "ShipUpgradesState.h"

#include <algorithm>
#include <array>
#include <string>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "campaign/CampaignSaveManager.h"
#include "gameplay/GameplayLaunch.h"
#include "localization/LocalizationManager.h"
#include "states/StateID.h"
#include "input/GamepadManager.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr sf::Color Cyan{ 25, 220, 255 };
	constexpr sf::Color Gold{ 255, 184, 55 };
	constexpr sf::Color MaximumGreen{ 55, 255, 135 };
	constexpr sf::Vector2f PartsPanelPosition{ 770.f, 170.f };
	constexpr sf::Vector2f PartsPanelSize{ 380.f, 70.f };
	constexpr float PartsContentGap{ 24.f };
	constexpr std::array<sf::Vector2f, 4> CardPositions{
		sf::Vector2f{ 280.f, 250.f }, sf::Vector2f{ 280.f, 400.f },
		sf::Vector2f{ 280.f, 550.f }, sf::Vector2f{ 280.f, 700.f }
	};
	constexpr sf::Vector2f CardSize{ 1360.f, 140.f };
	constexpr std::array<ShipUpgradeType, 4> UpgradeTypes{
		ShipUpgradeType::Armor, ShipUpgradeType::Engines,
		ShipUpgradeType::FireRate, ShipUpgradeType::BonusDuration
	};
	constexpr std::array<Config::Texture, 4> UpgradeIconTextures{
		Config::Texture::ShipUpgradeArmorIcon,
		Config::Texture::ShipUpgradeEnginesIcon,
		Config::Texture::ShipUpgradeFireRateIcon,
		Config::Texture::ShipUpgradeBonusDurationIcon };
	constexpr std::array<Config::Texture, 4> SelectedUpgradeIconTextures{
		Config::Texture::ShipUpgradeArmorIconSelected,
		Config::Texture::ShipUpgradeEnginesIconSelected,
		Config::Texture::ShipUpgradeFireRateIconSelected,
		Config::Texture::ShipUpgradeBonusDurationIconSelected };
	constexpr std::array<const char*, 4> NameKeys{
		"upgrades.armor", "upgrades.engines", "upgrades.fire_rate", "upgrades.bonus_duration" };
	constexpr std::array<const char*, 4> EffectKeys{
		"upgrades.armor_effect", "upgrades.engines_effect",
		"upgrades.fire_rate_effect", "upgrades.bonus_duration_effect" };
}

ShipUpgradesState::ShipUpgradesState(StateStack& stack, StateContext context)
	: State(stack, context)
	, background(context.assets.Textures().Get(Config::Texture::ShipUpgradesBackground))
	, headerDivider(context.assets.Textures().Get(Config::Texture::ShipUpgradesHeaderDivider))
	, glow(context.assets)
	, cursor(context.assets, Config::Texture::MenuPointer, { 6.f, 2.f }, Cyan)
	, fade(context.logicalSize)
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("upgrades.title"), 72)
	, partsPanel(context.assets.Fonts().Get(Config::Font::MenuRegular),
		context.assets.Textures().Get(Config::Texture::MenuButtonIdle),
		context.assets.Textures().Get(Config::Texture::MenuButtonSelected),
		"", PartsPanelSize)
	, partsLabel(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("upgrades.parts"), 31)
	, partsValue(context.assets.Fonts().Get(context.localization.GetBoldFont()), "0", 31)
	, partsIcon(context.assets.Textures().Get(Config::Texture::ShipUpgradesPartsIcon))
{
	returnToLevelSelect = context.gameplayLaunch.upgradesReturnToLevelSelect;
	context.window.setMouseCursorVisible(false);
	const sf::Vector2u backgroundSize{ background.getTexture().getSize() };
	const float backgroundScale{ std::max(
		context.logicalSize.x / static_cast<float>(backgroundSize.x),
		context.logicalSize.y / static_cast<float>(backgroundSize.y)) };
	background.setOrigin({ static_cast<float>(backgroundSize.x) * .5f,
		static_cast<float>(backgroundSize.y) * .5f });
	background.setScale({ backgroundScale, backgroundScale });
	background.setPosition(context.logicalSize * .5f);
	const sf::Vector2u dividerSize{ headerDivider.getTexture().getSize() };
	constexpr float DividerWidth{ 850.f };
	const float dividerScale{ DividerWidth / static_cast<float>(dividerSize.x) };
	headerDivider.setOrigin({ static_cast<float>(dividerSize.x) * .5f,
		static_cast<float>(dividerSize.y) * .5f });
	headerDivider.setScale({ dividerScale, dividerScale });
	headerDivider.setPosition({ context.logicalSize.x * .5f, 130.f });
	title.setFillColor(sf::Color(220, 249, 255));
	title.setOutlineColor(sf::Color(2, 12, 22, 230));
	title.setOutlineThickness(3.f);
	UI::TextLayout::CenterText(title, { 960.f, 58.f });
	partsPanel.SetPosition(PartsPanelPosition);
	partsPanel.SetFrameOpacity(.92f);
	partsLabel.setFillColor(Cyan);
	partsValue.setFillColor(sf::Color(225, 246, 255));
	const sf::Vector2u partsIconSize{ partsIcon.getTexture().getSize() };
	constexpr float PartsIconHeight{ 44.f };
	const float partsIconScale{ PartsIconHeight / static_cast<float>(partsIconSize.y) };
	partsIcon.setOrigin({ static_cast<float>(partsIconSize.x) * .5f,
		static_cast<float>(partsIconSize.y) * .5f });
	partsIcon.setScale({ partsIconScale, partsIconScale });
	LayoutPartsPanel();

	const sf::Font& headingFont{ context.assets.Fonts().Get(context.localization.GetBoldFont()) };
	const sf::Font& bodyFont{ context.assets.Fonts().Get(context.localization.GetRegularFont(false)) };
	const sf::Texture& rowFrame{
		context.assets.Textures().Get(Config::Texture::ShipUpgradesRowFrame) };
	const sf::Texture& selectedRowFrame{
		context.assets.Textures().Get(Config::Texture::ShipUpgradesRowFrameSelected) };
	const sf::Texture& partsIconTexture{
		context.assets.Textures().Get(Config::Texture::ShipUpgradesPartsIcon) };
	upgradeRows.reserve(UpgradeTypes.size());
	upgradeIcons.reserve(UpgradeTypes.size());
	for (std::size_t index{ 0 }; index < UpgradeTypes.size(); ++index)
	{
		upgradeRows.emplace_back(headingFont, rowFrame, selectedRowFrame, "", CardSize);
		upgradeRows.back().SetPosition(CardPositions[index]);
		upgradeRows.back().SetFrameOpacity(.96f);
		const sf::Texture& iconTexture{ context.assets.Textures().Get(UpgradeIconTextures[index]) };
		upgradeIcons.emplace_back(iconTexture);
		const sf::Vector2u iconSize{ iconTexture.getSize() };
		constexpr float IconExtent{ 106.f };
		const float iconScale{ IconExtent / static_cast<float>(std::max(iconSize.x, iconSize.y)) };
		upgradeIcons.back().setOrigin({ static_cast<float>(iconSize.x) * .5f,
			static_cast<float>(iconSize.y) * .5f });
		upgradeIcons.back().setScale({ iconScale, iconScale });
		upgradeIcons.back().setPosition(CardPositions[index] + sf::Vector2f{ 82.f, 70.f });
		cardTitles.emplace_back(headingFont, context.localization.GetText(NameKeys[index]), 29);
		UI::TextLayout::FitWidth(cardTitles.back(), 420.f, 20u);
		cardTitles.back().setFillColor(Cyan);
		cardTitles.back().setOutlineColor(sf::Color(0, 95, 145, 150));
		cardTitles.back().setOutlineThickness(2.f);
		cardTitles.back().setPosition(CardPositions[index] + sf::Vector2f{ 160.f, 24.f });
		cardDetails.emplace_back(bodyFont, context.localization.GetText(EffectKeys[index]), 29);
		UI::TextLayout::FitWidth(cardDetails.back(), 600.f, 19u);
		cardDetails.back().setFillColor(sf::Color(145, 190, 202));
		UI::TextLayout::CenterText(cardDetails.back(), CardPositions[index] + sf::Vector2f{ 745.f, 70.f });
		rankLabels.emplace_back(headingFont, context.localization.GetText("upgrades.rank"), 22);
		rankValues.emplace_back(headingFont, "0", 22);
		rankMaximums.emplace_back(headingFont, "/ 4", 22);
		rankLabels.back().setFillColor(sf::Color(225, 240, 246));
		rankValues.back().setFillColor(Cyan);
		rankMaximums.back().setFillColor(sf::Color(225, 240, 246));
		rankLabels.back().setPosition(CardPositions[index] + sf::Vector2f{ 160.f, 73.f });
		rankValues.back().setPosition(CardPositions[index] + sf::Vector2f{ 245.f, 73.f });
		rankMaximums.back().setPosition(CardPositions[index] + sf::Vector2f{ 272.f, 73.f });
		cardCosts.emplace_back(headingFont, "", 22);
		cardCosts.back().setPosition(CardPositions[index] + sf::Vector2f{ 1125.f, 27.f });
		costDividers.emplace_back(sf::Vector2f{ 2.f, 108.f });
		costDividers.back().setPosition(CardPositions[index] + sf::Vector2f{ 1060.f, 16.f });
		costDividers.back().setFillColor(sf::Color(35, 135, 174, 150));
		costIcons.emplace_back(partsIconTexture);
		const sf::Vector2u costIconSize{ partsIconTexture.getSize() };
		const float costIconScale{ PartsIconHeight / static_cast<float>(costIconSize.y) };
		costIcons.back().setOrigin({ static_cast<float>(costIconSize.x) * .5f,
			static_cast<float>(costIconSize.y) * .5f });
		costIcons.back().setScale({ costIconScale, costIconScale });
		costIcons.back().setPosition(CardPositions[index] + sf::Vector2f{ 1255.f, 42.f });
		upgradeButtons.emplace_back(headingFont, rowFrame, selectedRowFrame,
			context.localization.GetText("upgrades.upgrade"), sf::Vector2f{ 255.f, 54.f });
		upgradeButtons.back().SetPosition(CardPositions[index] + sf::Vector2f{ 1090.f, 73.f });
		upgradeButtons.back().SetFrameOpacity(.9f);
		upgradeButtons.back().SetLabelColor(Cyan);
		upgradeButtons.back().SetLabelOutline(sf::Color(0, 95, 145, 180), 2.f);
		maximumLabels.emplace_back(headingFont, context.localization.GetText("upgrades.max"), 42);
		maximumLabels.back().setFillColor(MaximumGreen);
		maximumLabels.back().setOutlineColor(sf::Color(0, 100, 52, 190));
		maximumLabels.back().setOutlineThickness(3.f);
		UI::TextLayout::CenterText(maximumLabels.back(), CardPositions[index] + sf::Vector2f{ 1210.f, 70.f });
	}

	const sf::Font& menuFont{ context.assets.Fonts().Get(context.localization.GetRegularFont()) };
	const sf::Texture& menuIdle{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
	const sf::Texture& menuSelected{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };
	buttons.emplace_back(menuFont, menuIdle, menuSelected,
		context.localization.GetText(returnToLevelSelect ? "upgrades.back_levels" : "upgrades.continue"),
		sf::Vector2f{ 540.f, 104.f });
	buttons.emplace_back(menuFont, menuIdle, menuSelected,
		context.localization.GetText("common.back_main"), sf::Vector2f{ 540.f, 104.f });
	buttons[0].SetPosition({ 1010.f, 885.f });
	buttons[1].SetPosition({ 370.f, 885.f });
	localizationRevision = context.localization.GetLanguageRevision();
	Refresh();
	std::size_t initialSelection{ 0u };
	while (initialSelection < upgradeRows.size() && maximumRanks[initialSelection])
		++initialSelection;
	Select(initialSelection, false);
	fade.StartFadeIn(.3f);
}

void ShipUpgradesState::OnReactivated()
{
	GetContext().window.setMouseCursorVisible(false);
	returnToLevelSelect = GetContext().gameplayLaunch.upgradesReturnToLevelSelect;
	if (localizationRevision != GetContext().localization.GetLanguageRevision())
		RefreshLocalizedContent();
	Refresh();
	std::size_t initialSelection{ 0u };
	while (initialSelection < upgradeRows.size() && maximumRanks[initialSelection])
		++initialSelection;
	Select(initialSelection, false);
	exitTarget = ExitTarget::None;
	fade.StartFadeIn(.3f);
}

void ShipUpgradesState::RefreshLocalizedContent()
{
	const StateContext& context{ GetContext() };
	localizationRevision = context.localization.GetLanguageRevision();

	const sf::Font& headingFont{ context.assets.Fonts().Get(context.localization.GetBoldFont()) };
	const sf::Font& bodyFont{ context.assets.Fonts().Get(context.localization.GetRegularFont(false)) };
	const sf::Font& menuFont{ context.assets.Fonts().Get(context.localization.GetRegularFont()) };

	title.setFont(headingFont);
	title.setString(context.localization.GetText("upgrades.title"));
	UI::TextLayout::CenterText(title, { 960.f, 58.f });

	partsLabel.setFont(headingFont);
	partsLabel.setString(context.localization.GetText("upgrades.parts"));
	LayoutPartsPanel();

	for (std::size_t index{ 0u }; index < UpgradeTypes.size(); ++index)
	{
		cardTitles[index].setFont(headingFont);
		cardTitles[index].setString(context.localization.GetText(NameKeys[index]));
		cardTitles[index].setScale({ 1.f, 1.f });
		UI::TextLayout::FitWidth(cardTitles[index], 420.f, 20u);

		cardDetails[index].setFont(bodyFont);
		cardDetails[index].setString(context.localization.GetText(EffectKeys[index]));
		cardDetails[index].setScale({ 1.f, 1.f });
		UI::TextLayout::FitWidth(cardDetails[index], 600.f, 19u);
		UI::TextLayout::CenterText(cardDetails[index], CardPositions[index] + sf::Vector2f{ 745.f, 70.f });

		rankLabels[index].setFont(headingFont);
		rankLabels[index].setString(context.localization.GetText("upgrades.rank"));

		upgradeButtons[index].SetFont(headingFont);
		upgradeButtons[index].SetLabel(context.localization.GetText("upgrades.upgrade"));

		maximumLabels[index].setFont(headingFont);
		maximumLabels[index].setString(context.localization.GetText("upgrades.max"));
		UI::TextLayout::CenterText(maximumLabels[index], CardPositions[index] + sf::Vector2f{ 1210.f, 70.f });
	}

	buttons[0].SetFont(menuFont);
	buttons[0].SetLabel(context.localization.GetText(
		returnToLevelSelect ? "upgrades.back_levels" : "upgrades.continue"));
	buttons[1].SetFont(menuFont);
	buttons[1].SetLabel(context.localization.GetText("common.back_main"));
}

void ShipUpgradesState::HandleEvent(const sf::Event& event)
{
	if (exitTarget != ExitTarget::None || fade.IsActive()) return;
	using enum GamepadManager::NavigationAction;
	switch (GetContext().gamepad.GetNavigationAction(event))
	{
	case Up: MoveVertical(-1); return;
	case Down: MoveVertical(1); return;
	case Left: MoveHorizontal(-1); return;
	case Right: MoveHorizontal(1); return;
	case Confirm: ActivateSelected(); return;
	case Back: BeginExit(returnToLevelSelect
		? ExitTarget::LevelSelect : ExitTarget::MainMenu); return;
	default: break;
	}
	if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
		for (std::size_t i{ 0 }; i < upgradeRows.size(); ++i)
			if (!maximumRanks[i] && upgradeRows[i].Contains(point)) Select(i);
		for (std::size_t i{ 0 }; i < buttons.size(); ++i)
			if (buttons[i].Contains(point)) Select(i + upgradeRows.size());
	}
	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		switch (key->code)
		{
		case sf::Keyboard::Key::Up: case sf::Keyboard::Key::W: MoveVertical(-1); return;
		case sf::Keyboard::Key::Down: case sf::Keyboard::Key::S: MoveVertical(1); return;
		case sf::Keyboard::Key::Left: case sf::Keyboard::Key::A: MoveHorizontal(-1); return;
		case sf::Keyboard::Key::Right: case sf::Keyboard::Key::D: MoveHorizontal(1); return;
		case sf::Keyboard::Key::Enter: case sf::Keyboard::Key::Space: ActivateSelected(); return;
		case sf::Keyboard::Key::Escape: BeginExit(returnToLevelSelect
			? ExitTarget::LevelSelect : ExitTarget::MainMenu); return;
		default: break;
		}
	}
	if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() };
		pressed && pressed->button == sf::Mouse::Button::Left)
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pressed->position) };
		for (std::size_t i{ 0 }; i < upgradeRows.size(); ++i)
			if (!maximumRanks[i] && upgradeRows[i].Contains(point))
			{ Select(i, false); ActivateSelected(); return; }
		for (std::size_t i{ 0 }; i < buttons.size(); ++i)
			if (buttons[i].Contains(point)) { Select(i + upgradeRows.size(), false); ActivateSelected(); return; }
	}
}

void ShipUpgradesState::Update(float deltaTime)
{
	glow.Update(deltaTime); cursor.Update(deltaTime); fade.Update(deltaTime);
	if (exitTarget == ExitTarget::None || fade.IsActive()) return;
	const ExitTarget target{ exitTarget }; exitTarget = ExitTarget::None;
	RequestClear();
	if (target == ExitTarget::Gameplay) RequestPush(StateID::Gameplay);
	else if (target == ExitTarget::LevelSelect)
	{
		// Rebuild the complete menu hierarchy after Gameplay cleared the old
		// stack. Level Select returns to Campaign Menu, which must still have the
		// real Main Menu beneath it.
		RequestPush(StateID::MainMenu);
		RequestPush(StateID::CampaignMenu);
		RequestPush(StateID::LevelSelect);
	}
	else RequestPush(StateID::MainMenu);
}

void ShipUpgradesState::Render()
{
	auto& window{ GetContext().window }; window.draw(background); window.draw(headerDivider);
	window.draw(title); partsPanel.Draw(window); window.draw(partsLabel);
	window.draw(partsValue); window.draw(partsIcon);
	for (std::size_t i{ 0 }; i < upgradeRows.size(); ++i)
	{
		if (!maximumRanks[i]) continue;
		glow.DrawBloom(window, upgradeRows[i].GetBounds(),
			[this, i](sf::RenderTarget& target, const sf::RenderStates& states)
			{ upgradeRows[i].Draw(target, states); }, MaximumGreen);
	}
	if (selectedIndex < upgradeRows.size())
	{
		const UI::MenuButton& selectedRow{ upgradeRows[selectedIndex] };
		glow.DrawBloom(window, selectedRow.GetBounds(),
			[&selectedRow](sf::RenderTarget& target, const sf::RenderStates& states)
			{ selectedRow.Draw(target, states); }, Gold);
	}
	for (std::size_t i{ 0 }; i < upgradeRows.size(); ++i)
	{
		upgradeRows[i].Draw(window); window.draw(upgradeIcons[i]); window.draw(cardTitles[i]);
		window.draw(rankLabels[i]); window.draw(rankValues[i]); window.draw(rankMaximums[i]);
		window.draw(cardDetails[i]); window.draw(costDividers[i]);
		if (maximumRanks[i]) window.draw(maximumLabels[i]);
		else
		{
			window.draw(cardCosts[i]); window.draw(costIcons[i]);
			upgradeButtons[i].Draw(window);
		}
	}
	for (std::size_t i{ 0 }; i < upgradeRows.size(); ++i)
		if (maximumRanks[i]) glow.DrawHighlight(window, upgradeRows[i].GetBounds(), MaximumGreen);
	if (selectedIndex < upgradeRows.size())
	{
		const UI::MenuButton& selectedRow{ upgradeRows[selectedIndex] };
		glow.DrawHighlight(window, selectedRow.GetBounds(), Gold);
	}
	else
	{
		const UI::MenuButton& selectedButton{ buttons[selectedIndex - upgradeRows.size()] };
		glow.DrawBloom(window, selectedButton.GetBounds(),
			[&selectedButton](sf::RenderTarget& target, const sf::RenderStates& states)
			{ selectedButton.Draw(target, states); }, Gold);
		glow.DrawHighlight(window, selectedButton.GetBounds(), Gold);
	}
	for (const UI::MenuButton& button : buttons) button.Draw(window);
}

void ShipUpgradesState::RenderOverlay()
{
	if (!GetContext().gamepad.IsInUse()) cursor.Draw(GetContext().window);
	fade.Draw(GetContext().window);
}

void ShipUpgradesState::Select(std::size_t index, bool playSound)
{
	if (index >= upgradeRows.size() + buttons.size()) return;
	if (index < upgradeRows.size() && maximumRanks[index]) return;
	const bool changed{ selectedIndex != index }; selectedIndex = index;
	for (std::size_t i{ 0 }; i < upgradeRows.size(); ++i)
		upgradeRows[i].SetSelected(index == i);
	for (std::size_t i{ 0 }; i < buttons.size(); ++i) buttons[i].SetSelected(index == i + upgradeRows.size());
	if (changed) glow.Invalidate();
	Refresh();
	if (changed && playSound)
		GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
}

void ShipUpgradesState::MoveVertical(int direction)
{
	// The upgrade cards form a single vertical column above one row of
	// buttons, so Up/Down only ever needs to walk that column or drop
	// straight into/out of the button row -- never sideways between the
	// two buttons (that's MoveHorizontal's job).
	if (selectedIndex < upgradeRows.size())
	{
		const int rowCount{ static_cast<int>(upgradeRows.size()) };
		int candidate{ static_cast<int>(selectedIndex) };
		for (int attempt{ 0 }; attempt < rowCount; ++attempt)
		{
			candidate += direction;
			if (candidate < 0 || candidate >= rowCount)
			{
				// Walked off the top or bottom of the list -- the button
				// row is the only thing left to land on.
				Select(upgradeRows.size());
				return;
			}
			if (!maximumRanks[static_cast<std::size_t>(candidate)])
			{
				Select(static_cast<std::size_t>(candidate));
				return;
			}
		}
		// Every card is already maxed out -- nothing in the column is
		// selectable, so fall through to the buttons.
		Select(upgradeRows.size());
		return;
	}

	// Currently on a button: there's only one row of those, so any
	// vertical move goes back up to the card list -- land on the last
	// selectable card, since that's the one closest to the buttons.
	for (int candidate{ static_cast<int>(upgradeRows.size()) - 1 }; candidate >= 0; --candidate)
	{
		if (!maximumRanks[static_cast<std::size_t>(candidate)])
		{
			Select(static_cast<std::size_t>(candidate));
			return;
		}
	}
}

void ShipUpgradesState::MoveHorizontal(int direction)
{
	// The card column has no horizontal neighbor -- Left/Right only does
	// something once the selection is already on the button row.
	if (selectedIndex < upgradeRows.size())
		return;

	const int buttonCount{ static_cast<int>(buttons.size()) };
	const int buttonIndex{ static_cast<int>(selectedIndex - upgradeRows.size()) };
	const int nextButtonIndex{ (buttonIndex + direction + buttonCount) % buttonCount };
	Select(upgradeRows.size() + static_cast<std::size_t>(nextButtonIndex));
}

void ShipUpgradesState::ActivateSelected()
{
	PlayPressSound();
	if (selectedIndex < upgradeRows.size())
	{
		if (!maximumRanks[selectedIndex]) Purchase(UpgradeTypes[selectedIndex]);
	}
	else if (selectedIndex == upgradeRows.size())
	{
		if (returnToLevelSelect)
		{
			BeginExit(ExitTarget::LevelSelect);
			return;
		}
		CampaignProgress* progress{ GetContext().campaignSave.EditProgress() };
		if (progress == nullptr) { BeginExit(ExitTarget::MainMenu); return; }
		const int availableLevels{ GetContext().assets.GetGameplayData().GetLevelCount() };
		const bool currentLevelCompleted{
			std::ranges::find(progress->completedLevels, progress->currentLevel) !=
				progress->completedLevels.end() };
		if (progress->currentLevel >= availableLevels && currentLevelCompleted)
		{
			progress->phase = CampaignPhase::Finished;
			static_cast<void>(GetContext().campaignSave.Save());
			BeginExit(ExitTarget::LevelSelect);
		}
		else
		{
			if (currentLevelCompleted)
			{
				++progress->currentLevel;
				progress->highestUnlockedLevel = std::max(
					progress->highestUnlockedLevel, progress->currentLevel);
			}
			progress->phase = CampaignPhase::Playing;
			static_cast<void>(GetContext().campaignSave.Save());
			GetContext().gameplayLaunch.mode = GameplayLaunchMode::ContinueCampaign;
			BeginExit(ExitTarget::Gameplay);
		}
	}
	else BeginExit(ExitTarget::MainMenu);
}

void ShipUpgradesState::Purchase(ShipUpgradeType type)
{
	CampaignProgress* progress{ GetContext().campaignSave.EditProgress() }; if (!progress) return;
	int& rank{ ShipUpgradeRules::GetRank(progress->upgrades, type) };
	if (rank >= ShipUpgradeRules::MaximumRank) return;
	const int cost{ ShipUpgradeRules::GetNextCost(rank) };
	if (progress->partsBalance < cost) return;
	progress->partsBalance -= cost; ++rank;
	if (!GetContext().campaignSave.Save())
	{
		--rank;
		progress->partsBalance += cost;
	}
	Refresh();
	if (rank >= ShipUpgradeRules::MaximumRank && selectedIndex < upgradeRows.size())
		MoveVertical(1);
}

void ShipUpgradesState::Refresh()
{
	const CampaignProgress* progress{ GetContext().campaignSave.GetProgress() };
	if (!progress) return;
	partsValue.setString(std::to_string(progress->partsBalance));
	LayoutPartsPanel();
	for (std::size_t i{ 0 }; i < upgradeRows.size(); ++i)
	{
		const int rank{ ShipUpgradeRules::GetRank(progress->upgrades, UpgradeTypes[i]) };
		const bool maximum{ rank >= ShipUpgradeRules::MaximumRank };
		maximumRanks[i] = maximum;
		const int cost{ ShipUpgradeRules::GetNextCost(rank) };
		rankValues[i].setString(std::to_string(rank));
		cardCosts[i].setString(GetContext().localization.FormatText(
			"upgrades.cost", "cost", std::to_string(cost)));
		cardCosts[i].setFillColor(progress->partsBalance >= cost ?
			sf::Color(225, 240, 246) : sf::Color(125, 135, 145));
		upgradeRows[i].SetSelected(!maximum && selectedIndex == i);
		upgradeRows[i].SetFrameColor(maximum ? MaximumGreen : sf::Color::White);
		const bool selected{ !maximum && selectedIndex == i };
		upgradeButtons[i].SetSelected(selected);
		upgradeButtons[i].SetLabelColor(selected ? Gold : Cyan);
		upgradeButtons[i].SetLabelOutline(selected ? sf::Color(135, 70, 0, 210) :
			sf::Color(0, 95, 145, 180), 2.f);
		upgradeIcons[i].setTexture(GetContext().assets.Textures().Get(
			!maximum && selectedIndex == i ? SelectedUpgradeIconTextures[i] : UpgradeIconTextures[i]), false);
		upgradeIcons[i].setColor(maximum ? MaximumGreen : sf::Color::White);
		cardTitles[i].setFillColor(maximum ? MaximumGreen :
			(selectedIndex == i ? Gold : Cyan));
		rankValues[i].setFillColor(maximum ? MaximumGreen :
			(selectedIndex == i ? Gold : Cyan));
	}
}

void ShipUpgradesState::LayoutPartsPanel()
{
	const float labelWidth{ partsLabel.getLocalBounds().size.x };
	const float valueWidth{ partsValue.getLocalBounds().size.x };
	const float iconWidth{ partsIcon.getGlobalBounds().size.x };
	const float contentWidth{ labelWidth + valueWidth + iconWidth + PartsContentGap * 2.f };
	const float contentLeft{ PartsPanelPosition.x + (PartsPanelSize.x - contentWidth) * .5f };
	const float centerY{ PartsPanelPosition.y + PartsPanelSize.y * .5f };

	UI::TextLayout::CenterText(partsLabel, { contentLeft + labelWidth * .5f, centerY });
	UI::TextLayout::CenterText(partsValue, {
		contentLeft + labelWidth + PartsContentGap + valueWidth * .5f, centerY });
	partsIcon.setPosition({
		contentLeft + labelWidth + PartsContentGap + valueWidth + PartsContentGap + iconWidth * .5f,
		centerY });
}

void ShipUpgradesState::BeginExit(ExitTarget target)
{
	GetContext().gameplayLaunch.upgradesReturnToLevelSelect = false;
	exitTarget = target; fade.StartFadeOut(.38f);
}

void ShipUpgradesState::PlayPressSound()
{
	GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
}
