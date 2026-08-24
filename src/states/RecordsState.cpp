#include "RecordsState.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "records/RecordsManager.h"
#include "localization/LocalizationManager.h"
#include "input/GamepadManager.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr sf::Color Cyan{ 25, 220, 255 };
	constexpr sf::Color SelectionGold{ 255, 178, 42 };
	constexpr sf::Color HeadingColor{ 215, 247, 252 };
	constexpr sf::Vector2f CampaignPosition{ 160.f, 210.f };
	constexpr sf::Vector2f CampaignSize{ 760.f, 690.f };
	constexpr sf::Vector2f HordePosition{ 1000.f, 210.f };
	constexpr sf::Vector2f HordeSize{ 760.f, 260.f };
	constexpr sf::Vector2f RunPosition{ 1000.f, 600.f };
	constexpr sf::Vector2f RunSize{ 760.f, 300.f };
	constexpr float FadeDuration{ .3f };

	void CenterText(sf::Text& text, sf::Vector2f position)
	{
		const sf::FloatRect bounds{ text.getLocalBounds() };
		text.setOrigin({ bounds.position.x + bounds.size.x * .5f,
			bounds.position.y + bounds.size.y * .5f });
		text.setPosition(position);
	}

	void AlignLeft(sf::Text& text, sf::Vector2f position)
	{
		const sf::FloatRect bounds{ text.getLocalBounds() };
		text.setOrigin({ bounds.position.x, bounds.position.y + bounds.size.y * .5f });
		text.setPosition(position);
	}

	void AlignRight(sf::Text& text, sf::Vector2f position)
	{
		const sf::FloatRect bounds{ text.getLocalBounds() };
		text.setOrigin({ bounds.position.x + bounds.size.x,
			bounds.position.y + bounds.size.y * .5f });
		text.setPosition(position);
	}

	std::string FormatDuration(int seconds)
	{
		std::ostringstream stream;
		stream << std::setfill('0') << std::setw(2) << seconds / 60
			<< ':' << std::setw(2) << seconds % 60;
		return stream.str();
	}
}

RecordsState::RecordsState(StateStack& stack, StateContext context)
	: State(stack, context)
	, background(context.assets, context.logicalSize)
	, titleGlow(context.assets)
	, buttonGlow(context.assets)
	, cursor(context.assets, Config::Texture::MenuPointer, { 6.f, 2.f }, Cyan)
	, fade(context.logicalSize)
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()), context.localization.GetText("records.title"), 72)
	, campaignPanel(CampaignSize, 22.f, 12u)
	, hordePanel(HordeSize, 22.f, 12u)
	, runPanel(RunSize, 22.f, 12u)
	, campaignTitle(context.assets.Fonts().Get(context.localization.GetBoldFont()), context.localization.GetText("campaign_menu.title"), 42)
	, hordeTitle(context.assets.Fonts().Get(context.localization.GetBoldFont()), context.localization.GetText("campaign_menu.horde"), 42)
	, runTitle(context.assets.Fonts().Get(context.localization.GetBoldFont()), context.localization.GetText("campaign_menu.run"), 42)
	, runLabel(context.assets.Fonts().Get(context.localization.GetCurrentLanguage() == Language::English ? Config::Font::BodyRegular : context.localization.GetRegularFont()), context.localization.GetText("records.best_time"), 27)
	, runValue(context.assets.Fonts().Get(context.localization.GetRegularFont()), "00:00", 48)
	, returnButton(context.assets.Fonts().Get(context.localization.GetRegularFont()),
		context.assets.Textures().Get(Config::Texture::MenuButtonIdle),
		context.assets.Textures().Get(Config::Texture::MenuButtonSelected),
		"", { 540.f, 104.f })
{
	context.window.setMouseCursorVisible(false);
	title.setFillColor(HeadingColor);
	title.setOutlineColor(sf::Color(3, 18, 31, 235));
	title.setOutlineThickness(3.5f);
	title.setLetterSpacing(1.12f);
	CenterText(title, { 960.f, 82.f });

	for (auto* panel : { &campaignPanel, &hordePanel, &runPanel })
	{
		panel->setFillColor(sf::Color(2, 13, 27, 225));
		panel->setOutlineColor(Cyan);
		panel->setOutlineThickness(2.f);
	}
	campaignPanel.setPosition(CampaignPosition);
	hordePanel.setPosition(HordePosition);
	runPanel.setPosition(RunPosition);
	for (sf::Text* heading : { &campaignTitle, &hordeTitle, &runTitle })
	{
		heading->setFillColor(HeadingColor);
		heading->setOutlineColor(sf::Color(3, 18, 31, 235));
		heading->setOutlineThickness(2.f);
	}
	CenterText(campaignTitle, { 540.f, 170.f });
	CenterText(hordeTitle, { 1380.f, 170.f });
	CenterText(runTitle, { 1380.f, 535.f });

	const sf::Font& bodyFont{ context.assets.Fonts().Get(context.localization.GetCurrentLanguage() == Language::English ? Config::Font::BodyRegular : context.localization.GetRegularFont()) };
	const sf::Font& menuFont{ context.assets.Fonts().Get(context.localization.GetRegularFont()) };
	levelLabels.reserve(10u); levelScores.reserve(10u);
	for (std::size_t index{ 0 }; index < 10u; ++index)
	{
		const int level{ static_cast<int>(index) + 1 };
		const float y{ CampaignPosition.y + 70.f + static_cast<float>(index) * 58.f };
		sf::String levelLabel{ context.localization.GetText("records.level") };
		levelLabel += " " + std::to_string(level);
		levelLabels.emplace_back(bodyFont, levelLabel, 27);
		levelScores.emplace_back(menuFont,
			std::to_string(context.records.GetCampaignLevelScore(level)), 29);
		levelLabels.back().setFillColor(sf::Color(205, 230, 238));
		levelScores.back().setFillColor(Cyan);
		AlignLeft(levelLabels.back(), { CampaignPosition.x + 70.f, y });
		AlignRight(levelScores.back(), { CampaignPosition.x + CampaignSize.x - 70.f, y });
	}
	const GameRecords& records{ context.records.GetRecords() };
	const std::array<sf::String, 2> hordeNames{
		context.localization.GetText("records.waves_survived"), context.localization.GetText("records.best_score") };
	const std::array<int, 2> hordeNumbers{ records.hordeWaves, records.hordeScore };
	hordeLabels.reserve(2u); hordeValues.reserve(2u);
	for (std::size_t index{ 0 }; index < 2u; ++index)
	{
		hordeLabels.emplace_back(bodyFont, hordeNames[index], 29);
		hordeValues.emplace_back(menuFont, std::to_string(hordeNumbers[index]), 34);
		hordeLabels.back().setFillColor(sf::Color(205, 230, 238));
		hordeValues.back().setFillColor(Cyan);
		const float y{ HordePosition.y + 85.f + static_cast<float>(index) * 90.f };
		AlignLeft(hordeLabels.back(), { HordePosition.x + 70.f, y });
		AlignRight(hordeValues.back(), { HordePosition.x + HordeSize.x - 70.f, y });
	}
	runLabel.setFillColor(sf::Color(205, 230, 238));
	runValue.setFillColor(Cyan);
	runValue.setString(FormatDuration(records.runSeconds));
	CenterText(runLabel, { 1380.f, RunPosition.y + 95.f });
	CenterText(runValue, { 1380.f, RunPosition.y + 195.f });
	returnButton.SetPosition({ 690.f, 935.f });
	returnButton.SetLabel(context.localization.GetText("common.back_main"));
	returnButton.SetSelected(false);
	fade.StartFadeIn(FadeDuration);
}

void RecordsState::HandleEvent(const sf::Event& event)
{
	if (returning || fade.IsActive()) return;
	const auto navigation{ GetContext().gamepad.GetNavigationAction(event) };
	if (navigation == GamepadManager::NavigationAction::Confirm ||
		navigation == GamepadManager::NavigationAction::Back)
	{
		BeginReturn(); return;
	}
	if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
		const bool wasSelected{ returnButtonSelected };
		returnButtonSelected = returnButton.Contains(point);
		returnButton.SetSelected(returnButtonSelected);
		if (returnButtonSelected != wasSelected)
			buttonGlow.Invalidate();
		return;
	}
	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		if (key->code == sf::Keyboard::Key::Escape || key->code == sf::Keyboard::Key::Backspace ||
			key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
		{ BeginReturn(); return; }
	}
	if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() };
		pressed && pressed->button == sf::Mouse::Button::Left)
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pressed->position) };
		if (returnButton.Contains(point)) BeginReturn();
	}
}

void RecordsState::Update(float deltaTime)
{
	background.Update(deltaTime);
	titleGlow.Update(deltaTime);
	buttonGlow.Update(deltaTime);
	cursor.Update(deltaTime);
	fade.Update(deltaTime);
	if (GetContext().gamepad.IsInUse() && !returnButtonSelected)
	{
		returnButtonSelected = true;
		returnButton.SetSelected(true);
		buttonGlow.Invalidate();
	}
	if (returning && !fade.IsActive()) RequestPop();
}

void RecordsState::Render()
{
	auto& window{ GetContext().window };
	background.Draw(window);
	titleGlow.DrawBloom(window, title.getGlobalBounds(),
		[this](sf::RenderTarget& target, const sf::RenderStates& states)
		{ target.draw(title, states); }, Cyan);
	window.draw(title);
	for (const RoundedRectangleShape* panel : { &campaignPanel, &hordePanel, &runPanel })
		window.draw(*panel);
	window.draw(campaignTitle); window.draw(hordeTitle); window.draw(runTitle);
	for (std::size_t index{ 0 }; index < levelLabels.size(); ++index)
	{ window.draw(levelLabels[index]); window.draw(levelScores[index]); }
	for (std::size_t index{ 0 }; index < hordeLabels.size(); ++index)
	{ window.draw(hordeLabels[index]); window.draw(hordeValues[index]); }
	window.draw(runLabel); window.draw(runValue);
	if (returnButtonSelected)
		buttonGlow.DrawBloom(window, returnButton.GetBounds(),
			[this](sf::RenderTarget& target, const sf::RenderStates& states)
			{ returnButton.Draw(target, states); }, SelectionGold);
	returnButton.Draw(window);
	if (returnButtonSelected)
		buttonGlow.DrawHighlight(window, returnButton.GetBounds(), SelectionGold);
}

void RecordsState::RenderOverlay()
{
	if (!GetContext().gamepad.IsInUse()) cursor.Draw(GetContext().window);
	fade.Draw(GetContext().window);
}

void RecordsState::BeginReturn()
{
	GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI,
		100.f, 1.f, SoundPlayback::StopPrevious);
	returning = true;
	fade.StartFadeOut(FadeDuration);
}
