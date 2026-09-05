#include "OptionsState.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "app/DisplayManager.h"
#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "settings/SettingsManager.h"
#include "input/gamepad/GamepadManager.h"
#include "ui/MenuTheme.h"
#include "ui/OptionsWidgets.h"
#include "ui/TextLayout.h"

namespace
{
	constexpr sf::Color Cyan{ 105, 225, 242 };
	constexpr sf::Color BrightCyan{ 205, 250, 255 };
	constexpr sf::Color Orange{ 255, 190, 72 };
	constexpr sf::Color Disabled{ 76, 88, 101 };
	constexpr sf::Color Red{ 245, 92, 92 };
	constexpr float StateFadeDuration{ 0.24f };
	constexpr float PageFadeOutDuration{ 0.1f };
	constexpr float PageFadeInDuration{ 0.14f };
	constexpr sf::Vector2f RowPosition{ 260.f, 220.f };
	constexpr sf::Vector2f RowSize{ 1400.f, 82.f };
	constexpr float RowSpacing{ 98.f };
	constexpr std::array<unsigned int, 7> FrameLimits{ 0u, 30u, 60u, 120u, 144u, 240u, 360u };
	const sf::FloatRect DialogConfirmBounds({ 690.f, 580.f }, { 250.f, 58.f });
	const sf::FloatRect DialogCancelBounds({ 980.f, 580.f }, { 250.f, 58.f });
	const sf::FloatRect BindingCancelBounds({ 835.f, 570.f }, { 250.f, 58.f });

}

OptionsState::OptionsState(StateStack& stateStack, StateContext context, Origin optionsOrigin)
	: MenuState(stateStack, context)
	, shade(context.logicalSize)
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()), context.localization.GetText("options.title"), 76)
	, titleGlow(context.assets)
	, neonGlow(context.assets)
	, dialogGlow(context.assets)
	, toggleOnText(context.assets.Fonts().Get(context.localization.GetRegularFont()), context.localization.GetText("common.on"), 23)
	, toggleOffText(context.assets.Fonts().Get(context.localization.GetRegularFont()), context.localization.GetText("common.off"), 23)
	, previousGraphics(context.settings.GetSettings().graphics)
	, origin(optionsOrigin)
{
	context.window.setMouseCursorVisible(false);
	shade.setFillColor(sf::Color(0, 4, 10, 150));

	title.setFillColor(BrightCyan);
	title.setOutlineColor(sf::Color(2, 14, 25, 235));
	title.setOutlineThickness(3.f);
	ApplyPage(Page::Root);
	Chrome().StartFadeIn(StateFadeDuration);
}

void OptionsState::HandleEvent(const sf::Event& event)
{
	if (Chrome().IsFading())
		return;

	const GamepadManager::NavigationAction navigation{
		GetContext().gamepad.GetNavigationAction(event) };

	if (isDisplayConfirmationOpen)
	{
		using enum GamepadManager::NavigationAction;
		switch (navigation)
		{
		case Left: SelectDialogOption(0u); return;
		case Right: SelectDialogOption(1u); return;
		case Confirm: ActivateDialogOption(dialogSelectedIndex); return;
		case Back: ActivateDialogOption(1u); return;
		default: break;
		}

		if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
		{
			const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
			Chrome().SetMousePosition(point);
			if (DialogConfirmBounds.contains(point))
				SelectDialogOption(0u);
			else if (DialogCancelBounds.contains(point))
				SelectDialogOption(1u);
			return;
		}

		if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
		{
			if (mouse->button == sf::Mouse::Button::Left)
			{
				const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mouse->position) };
				if (DialogConfirmBounds.contains(point))
					ActivateDialogOption(0u);
				else if (DialogCancelBounds.contains(point))
					ActivateDialogOption(1u);
			}
			return;
		}

		if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
		{
			if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
				SelectDialogOption(0u);
			else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
				SelectDialogOption(1u);
			else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
				ActivateDialogOption(dialogSelectedIndex);
			else if (key->code == sf::Keyboard::Key::Escape)
				ActivateDialogOption(1u);
		}
		return;
	}

	if (pendingBinding.has_value())
	{
		if (navigation == GamepadManager::NavigationAction::Back)
		{
			GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI);
			pendingBinding.reset();
			return;
		}

		if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
		{
			Chrome().SetMousePosition(GetContext().window.mapPixelToCoords(moved->position));
			return;
		}

		if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
		{
			if (key->code == sf::Keyboard::Key::Escape)
				pendingBinding.reset();
			else if (key->code != sf::Keyboard::Key::Unknown)
				ApplyBinding({ RebindableInputDevice::Keyboard, static_cast<int>(key->code) });
		}
		else if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
		{
			const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mouse->position) };
			if (mouse->button == sf::Mouse::Button::Left && BindingCancelBounds.contains(point))
			{
				GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI);
				pendingBinding.reset();
			}
			else
			{
				ApplyBinding({ RebindableInputDevice::Mouse, static_cast<int>(mouse->button) });
			}
		}
		return;
	}

	if (isDropdownOpen)
	{
		using enum GamepadManager::NavigationAction;
		switch (navigation)
		{
		case Up: MoveDropdownSelection(-1); return;
		case Down: MoveDropdownSelection(1); return;
		case Confirm: ApplyDropdownSelection(); return;
		case Back: CloseDropdown(); return;
		default: break;
		}

		if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
		{
			if (key->code == sf::Keyboard::Key::Up)
				MoveDropdownSelection(-1);
			else if (key->code == sf::Keyboard::Key::Down)
				MoveDropdownSelection(1);
			else if (key->code == sf::Keyboard::Key::Enter)
				ApplyDropdownSelection();
			else if (key->code == sf::Keyboard::Key::Escape)
				CloseDropdown();
		}
		else if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
		{
			const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
			Chrome().SetMousePosition(point);
			if (isDropdownScrollbarDragging)
				UpdateDropdownScrollbar(point);
			else
				HandleDropdownMouseMove(point);
		}
		else if (const auto* wheel{ event.getIf<sf::Event::MouseWheelScrolled>() })
		{
			HandleMouseWheel(wheel->delta);
		}
		else if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
		{
			if (mouse->button == sf::Mouse::Button::Left)
				HandleDropdownMousePress(
					GetContext().window.mapPixelToCoords(mouse->position));
		}
		else if (event.is<sf::Event::MouseButtonReleased>())
		{
			isDropdownScrollbarDragging = false;
		}
		return;
	}

	if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
		Chrome().SetMousePosition(point);
		if (isSliderDragging)
			UpdateSliderFromMouse(point);
		else
			HandleMousePosition(moved->position);
		return;
	}

	if (event.is<sf::Event::MouseButtonReleased>())
	{
		isSliderDragging = false;
		return;
	}

	if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
	{
		if (mouse->button == sf::Mouse::Button::Left)
			HandleMousePress(mouse->position);
		return;
	}

	using enum GamepadManager::NavigationAction;
	switch (navigation)
	{
	case Up: SelectPrevious(); return;
	case Down: SelectNext(); return;
	case Left: AdjustSelected(-1); return;
	case Right: AdjustSelected(1); return;
	case Confirm: ActivateSelected(); return;
	case Back: Execute(Action::Back); return;
	default: break;
	}

	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		switch (key->code)
		{
		case sf::Keyboard::Key::Up:
		case sf::Keyboard::Key::W:
			SelectPrevious();
			break;
		case sf::Keyboard::Key::Down:
		case sf::Keyboard::Key::S:
			SelectNext();
			break;
		case sf::Keyboard::Key::Left:
		case sf::Keyboard::Key::A:
			AdjustSelected(-1);
			break;
		case sf::Keyboard::Key::Right:
		case sf::Keyboard::Key::D:
			AdjustSelected(1);
			break;
		case sf::Keyboard::Key::Enter:
		case sf::Keyboard::Key::Space:
			ActivateSelected();
			break;
		case sf::Keyboard::Key::Escape:
		case sf::Keyboard::Key::Backspace:
			Execute(Action::Back);
			break;
		default:
			break;
		}
	}
}

void OptionsState::OnUpdate(float deltaTime)
{
	titleGlow.Update(deltaTime);
	neonGlow.Update(deltaTime);
	dialogGlow.Update(deltaTime);

	// A finished exit fade is handled by MenuState (BeginTransition). What is
	// left here is the page-to-page fade, which swaps content mid-fade and
	// then fades back in without ever leaving the state.
	if (!Chrome().IsFading() && pendingPage.has_value())
	{
		const Page nextPage{ *pendingPage };
		pendingPage.reset();
		ApplyPage(nextPage);
		Chrome().StartFadeIn(PageFadeInDuration);
	}

	if (!isDisplayConfirmationOpen)
		return;

	displayConfirmationRemaining -= deltaTime;
	if (displayConfirmationRemaining <= 0.f)
		RevertDisplayChange();
}

void OptionsState::OnRender()
{
	sf::RenderWindow& window{ GetContext().window };
	window.draw(shade);
	DrawTitle(window);
	if (page == Page::GamepadControls)
		DrawGamepadLayouts(window);
	DrawRows(window);
	if (isDropdownOpen)
		DrawDropdown(window);
	if (pendingBinding.has_value() || isDisplayConfirmationOpen)
		DrawDialog(window);
}

void OptionsState::RenderOverlay()
{
	// The cursor is hidden during any fade (entrance, exit, or a page swap),
	// which the shared MenuChrome::DrawOverlay wouldn't do -- hence the
	// override rather than the base behaviour.
	sf::RenderWindow& window{ GetContext().window };
	if (!Chrome().IsFading() && !GetContext().gamepad.IsInUse())
		Chrome().Cursor().Draw(window);
	Chrome().Fade().Draw(window);
}

void OptionsState::ApplyPage(Page newPage)
{
	page = newPage;
	isGamepadLayoutCacheDirty = true;
	RefreshTitle();
	selectedIndex = 0u;
	isDropdownOpen = false;
	pendingBinding.reset();
	RebuildRows();
}

void OptionsState::BeginPageTransition(Page newPage)
{
	if (newPage == page || pendingPage.has_value() || IsTransitioning())
		return;
	pendingPage = newPage;
	Chrome().StartFadeOut(PageFadeOutDuration);
}

void OptionsState::BeginExit()
{
	if (IsTransitioning() || pendingPage.has_value())
		return;
	BeginTransition(StateFadeDuration, [this] { RequestPop(); });
}

OptionsState::~OptionsState()
{
	if (GetContext().window.isOpen())
		GetContext().window.setMouseCursorVisible(false);
}

void OptionsState::RefreshTitle()
{
	const auto& localize{ GetContext().localization };
	sf::String value;
	switch (page)
	{
	case Page::Root: value = localize.GetText("options.title"); break;
	case Page::Graphics: value = localize.GetText("options.graphics"); break;
	case Page::Audio: value = localize.GetText("options.audio"); break;
	case Page::Gameplay: value = localize.GetText("options.gameplay"); break;
	case Page::Controls: value = localize.GetText("options.controls"); break;
	case Page::Language: value = localize.GetText("options.language"); break;
	case Page::KeyboardControls: value = localize.GetText("options.keyboard"); break;
	case Page::GamepadControls: value = localize.GetText("options.gamepad"); break;
	}
	const Language language{ localize.GetCurrentLanguage() };
	const auto titleFont{ language == Language::English ? Config::Font::MenuSemibold
		: (language == Language::Arabic ? Config::Font::ArabicBold : Config::Font::LocalizedBold) };
	title.setFont(GetContext().assets.Fonts().Get(titleFont));

	const auto center{ [this, &value](sf::Text& text)
		{
			text.setString(value);
			const sf::FloatRect bounds{ text.getLocalBounds() };
			text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
				bounds.position.y + bounds.size.y * 0.5f });
			text.setPosition({ GetContext().logicalSize.x * 0.5f, 105.f });
		} };
	center(title);
	titleGlow.Invalidate();
}

void OptionsState::RebuildRows()
{
	rows.clear();
	const auto add{ [this](sf::String label, RowKind kind, Action action, bool isEnabled = true)
		{
			const float y{ RowPosition.y + RowSpacing * static_cast<float>(rows.size()) };
			rows.push_back({ std::move(label), kind, action, isEnabled,
				sf::FloatRect({ RowPosition.x, y }, RowSize) });
		} };

	switch (page)
	{
	case Page::Root:
		add(GetContext().localization.GetText("options.graphics"), RowKind::Button, Action::OpenGraphics);
		add(GetContext().localization.GetText("options.audio"), RowKind::Button, Action::OpenAudio);
		add(GetContext().localization.GetText("options.gameplay"), RowKind::Button, Action::OpenGameplay);
		add(GetContext().localization.GetText("options.controls"), RowKind::Button, Action::OpenControls);
		add(GetContext().localization.GetText("options.language"), RowKind::Button, Action::OpenLanguage);
		add(GetContext().localization.GetText("options.restore_defaults"), RowKind::Button, Action::ResetAll);
		add(GetContext().localization.GetText(origin == Origin::PauseMenu
			? "options.back_pause" : "options.back_main"),
			RowKind::Button, Action::Back);
		break;
	case Page::Language:
		add(LocalizationManager::GetLanguageNativeName(Language::English), RowKind::Button, Action::SetEnglish);
		add(LocalizationManager::GetLanguageNativeName(Language::Spanish), RowKind::Button, Action::SetSpanish);
		add(LocalizationManager::GetLanguageNativeName(Language::Russian), RowKind::Button, Action::SetRussian);
		add(LocalizationManager::GetLanguageNativeName(Language::Ukrainian), RowKind::Button, Action::SetUkrainian);
		add(LocalizationManager::GetLanguageNativeName(Language::Arabic), RowKind::Button, Action::SetArabic);
		add(GetContext().localization.GetText("options.back"), RowKind::Button, Action::Back);
		break;
	case Page::Graphics:
		add(GetContext().localization.GetText("options.display_resolution"), RowKind::Dropdown, Action::Resolution,
			GetContext().settings.GetSettings().graphics.windowMode != WindowMode::Borderless);
		add(GetContext().localization.GetText("options.window_mode"), RowKind::Dropdown, Action::WindowMode);
		add(GetContext().localization.GetText("options.show_fps"), RowKind::Toggle, Action::ShowFps);
		add(GetContext().localization.GetText("options.vsync"), RowKind::Toggle, Action::VerticalSync);
		add(GetContext().localization.GetText("options.frame_limit"), RowKind::Choice, Action::FrameRateLimit,
			!GetContext().settings.GetSettings().graphics.isVSyncEnabled);
		add(GetContext().localization.GetText("options.post_effects"), RowKind::Toggle, Action::PostEffects);
		add(GetContext().localization.GetText("options.reset_graphics"), RowKind::Button, Action::ResetGraphics);
		add(GetContext().localization.GetText("options.back"), RowKind::Button, Action::Back);
		break;
	case Page::Gameplay:
		add(GetContext().localization.GetText("options.screen_shake"), RowKind::Toggle, Action::ScreenShake);
		add(GetContext().localization.GetText("options.score_popups"), RowKind::Toggle, Action::ShowScorePopups);
		add(GetContext().localization.GetText("options.gamepad_vibration"), RowKind::Toggle, Action::GamepadVibration);
		add(GetContext().localization.GetText("options.gamepad_adaptive_triggers"), RowKind::Toggle, Action::GamepadAdaptiveTriggers);
		add(GetContext().localization.GetText("options.gamepad_lightbar"), RowKind::Toggle, Action::GamepadLightbar);
		add(GetContext().localization.GetText("options.reset_gameplay"), RowKind::Button, Action::ResetGameplay);
		add(GetContext().localization.GetText("options.back"), RowKind::Button, Action::Back);
		break;
	case Page::Audio:
		add(GetContext().localization.GetText("options.music"), RowKind::Slider, Action::MusicVolume);
		add(GetContext().localization.GetText("options.sounds"), RowKind::Slider, Action::SoundVolume);
		add(GetContext().localization.GetText("options.reset_audio"), RowKind::Button, Action::ResetAudio);
		add(GetContext().localization.GetText("options.back"), RowKind::Button, Action::Back);
		break;
	case Page::Controls:
		add(GetContext().localization.GetText("options.keyboard"), RowKind::Button, Action::OpenKeyboardControls);
		add(GetContext().localization.GetText("options.gamepad"), RowKind::Button, Action::OpenGamepadControls);
		add(GetContext().localization.GetText("options.back"), RowKind::Button, Action::Back);
		break;
	case Page::KeyboardControls:
		add(GetContext().localization.GetText("options.move_up"), RowKind::Binding, Action::MoveUp);
		add(GetContext().localization.GetText("options.move_down"), RowKind::Binding, Action::MoveDown);
		add(GetContext().localization.GetText("options.move_left"), RowKind::Binding, Action::MoveLeft);
		add(GetContext().localization.GetText("options.move_right"), RowKind::Binding, Action::MoveRight);
		add(GetContext().localization.GetText("options.fire"), RowKind::Binding, Action::Fire);
		add(GetContext().localization.GetText("options.reset_controls"), RowKind::Button, Action::ResetControls);
		add(GetContext().localization.GetText("options.back"), RowKind::Button, Action::Back);
		break;
	case Page::GamepadControls:
		add(GetContext().localization.GetText("options.back_controls"), RowKind::Button, Action::Back);
		rows.back().bounds.position.x = 580.f;
		rows.back().bounds.position.y = 880.f;
		rows.back().bounds.size.x = 760.f;
		break;
	}

	if (!rows.empty() && !rows[selectedIndex].isEnabled)
		SelectNext();

	RebuildRowTextCache();
	neonGlow.Invalidate();
}

void OptionsState::RebuildRowTextCache()
{
	const Language language{ GetContext().localization.GetCurrentLanguage() };
	const auto defaultFontID{ language == Language::English ? Config::Font::MenuRegular
		: (language == Language::Arabic ? Config::Font::ArabicRegular
			: Config::Font::LocalizedRegular) };
	const sf::Font& font{ GetContext().assets.Fonts().Get(defaultFontID) };
	toggleOnText.setFont(font);
	toggleOffText.setFont(font);
	toggleOnText.setString(GetContext().localization.GetText("common.on"));
	toggleOffText.setString(GetContext().localization.GetText("common.off"));
	rowLabels.clear();
	rowValues.clear();
	rowHints.clear();
	rowLabels.reserve(rows.size());
	rowValues.reserve(rows.size());
	rowHints.reserve(rows.size());

	for (const Row& row : rows)
	{
		const bool arabicLanguageRow{ page == Page::Language && row.action == Action::SetArabic };
		const bool nativeLanguageRow{ page == Page::Language && row.action != Action::Back };
		const sf::Font& rowFont{ GetContext().assets.Fonts().Get(arabicLanguageRow
			? Config::Font::ArabicRegular
			: (nativeLanguageRow ? Config::Font::LocalizedRegular : defaultFontID)) };
		rowLabels.emplace_back(rowFont, row.label, 30);
		UI::TextLayout::FitWidth(rowLabels.back(), 760.f, 19u);
		rowLabels.back().setPosition(row.bounds.position + sf::Vector2f{ 34.f, 20.f });

		rowValues.emplace_back(font, GetRowValue(row), 28);
		rowHints.emplace_back(font, "", 14);
		if (row.kind == RowKind::Dropdown)
		{
			const sf::FloatRect bounds{ UI::OptionsWidgets::GetValueBoxBounds(row.bounds.position.y) };
			rowValues.back().setCharacterSize(row.isEnabled ? 25u : 21u);
			UI::TextLayout::FitWidth(rowValues.back(), bounds.size.x - 40.f, 16u);
			rowValues.back().setPosition(
				bounds.position + sf::Vector2f{ 20.f, row.isEnabled ? 13.f : 5.f });
			if (!row.isEnabled)
			{
				rowHints.back().setString(GetContext().localization.GetText("options.desktop_controlled"));
				UI::TextLayout::FitWidth(rowHints.back(), bounds.size.x - 40.f, 11u);
				rowHints.back().setPosition(bounds.position + sf::Vector2f{ 20.f, 33.f });
				rowHints.back().setFillColor(Red);
			}
		}
		else if (row.kind == RowKind::Slider)
		{
			rowValues.back().setCharacterSize(24u);
			rowValues.back().setPosition({ 1565.f, row.bounds.position.y + 23.f });
		}
		else
		{
			rowValues.back().setPosition({ 1160.f, row.bounds.position.y + 20.f });
		}

		if (page == Page::GamepadControls && row.action == Action::Back)
		{
			const sf::FloatRect labelBounds{ rowLabels.back().getLocalBounds() };
			rowLabels.back().setOrigin({
				labelBounds.position.x + labelBounds.size.x * 0.5f,
				labelBounds.position.y + labelBounds.size.y * 0.5f });
			rowLabels.back().setPosition({
				row.bounds.position.x + row.bounds.size.x * 0.5f,
				row.bounds.position.y + row.bounds.size.y * 0.5f });
		}
	}
}

void OptionsState::RefreshRowTextValues()
{
	if (rowValues.size() != rows.size())
	{
		RebuildRowTextCache();
		return;
	}

	for (std::size_t index{ 0u }; index < rows.size(); ++index)
	{
		rowValues[index].setString(GetRowValue(rows[index]));
		if (rows[index].kind == RowKind::Dropdown)
			UI::TextLayout::FitWidth(rowValues[index], UI::OptionsWidgets::GetValueBoxBounds(rows[index].bounds.position.y).size.x - 40.f, 16u);
	}

	neonGlow.Invalidate();
}

void OptionsState::Select(std::size_t index, bool playSound)
{
	if (index >= rows.size() || !rows[index].isEnabled)
		return;
	const bool changed{ selectedIndex != index };
	selectedIndex = index;
	if (changed)
		neonGlow.Invalidate();
	if (changed && playSound)
		GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f,
			SoundPlayback::StopPrevious);
}

void OptionsState::SelectPrevious()
{
	if (rows.empty())
		return;
	std::size_t index{ selectedIndex };
	do
	{
		index = index == 0u ? rows.size() - 1u : index - 1u;
	} while (!rows[index].isEnabled && index != selectedIndex);
	Select(index);
}

void OptionsState::SelectNext()
{
	if (rows.empty())
		return;
	std::size_t index{ selectedIndex };
	do
	{
		index = (index + 1u) % rows.size();
	} while (!rows[index].isEnabled && index != selectedIndex);
	Select(index);
}

void OptionsState::ActivateSelected()
{
	if (!IsSelectedRowEnabled())
		return;
	GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f,
		SoundPlayback::StopPrevious);

	const Row& row{ rows[selectedIndex] };
	if (row.kind == RowKind::Toggle || row.kind == RowKind::Choice)
		AdjustSelected(1);
	else if (row.kind == RowKind::Dropdown)
		OpenDropdown(row.action);
	else if (row.kind == RowKind::Binding)
		BeginBinding(row.action);
	else if (row.kind == RowKind::Button)
		Execute(row.action);
}

void OptionsState::AdjustSelected(int direction)
{
	if (!IsSelectedRowEnabled())
		return;

	const Action action{ rows[selectedIndex].action };
	GameSettings& settings{ GetContext().settings.EditSettings() };
	if (action == Action::MusicVolume || action == Action::SoundVolume)
	{
		float& value{ action == Action::MusicVolume
			? settings.audio.musicVolume
			: settings.audio.soundVolume };
		value = std::clamp(value + 5.f * static_cast<float>(direction), 0.f, 100.f);
		SaveAndApplyAudio();
	}
	else if (action == Action::ShowFps)
	{
		settings.graphics.needToShowFPS = !settings.graphics.needToShowFPS;
		SaveAndApplyLiveGraphics();
	}
	else if (action == Action::VerticalSync)
	{
		settings.graphics.isVSyncEnabled = !settings.graphics.isVSyncEnabled;
		SaveAndApplyLiveGraphics();
		RebuildRows();
	}
	else if (action == Action::PostEffects)
	{
		settings.graphics.arePostEffectsEnabled = !settings.graphics.arePostEffectsEnabled;
		SaveSettings();
	}
	else if (action == Action::ScreenShake)
	{
		settings.gameplay.isScreenShakeEnabled = !settings.gameplay.isScreenShakeEnabled;
		SaveSettings();
	}
	else if (action == Action::ShowScorePopups)
	{
		settings.gameplay.needToShowScorePopups = !settings.gameplay.needToShowScorePopups;
		SaveSettings();
	}
	else if (action == Action::GamepadVibration)
	{
		settings.gamepad.isVibrationEnabled = !settings.gamepad.isVibrationEnabled;
		SaveSettings();
	}
	else if (action == Action::GamepadAdaptiveTriggers)
	{
		settings.gamepad.isAdaptiveTriggersEnabled = !settings.gamepad.isAdaptiveTriggersEnabled;
		SaveSettings();
	}
	else if (action == Action::GamepadLightbar)
	{
		settings.gamepad.isControllerLightbarEnabled = !settings.gamepad.isControllerLightbarEnabled;
		SaveSettings();
	}
	else if (action == Action::FrameRateLimit)
	{
		auto iterator{ std::ranges::find(FrameLimits, settings.graphics.frameRateLimit) };
		std::size_t index{ iterator == FrameLimits.end()
			? 0u
			: static_cast<std::size_t>(std::distance(FrameLimits.begin(), iterator)) };
		index = direction > 0
			? (index + 1u) % FrameLimits.size()
			: (index == 0u ? FrameLimits.size() - 1u : index - 1u);
		settings.graphics.frameRateLimit = FrameLimits[index];
		SaveAndApplyLiveGraphics();
	}

	RefreshRowTextValues();
}

void OptionsState::HandleMousePosition(sf::Vector2i pixelPosition)
{
	const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pixelPosition) };
	for (std::size_t index{ 0u }; index < rows.size(); ++index)
	{
		if (rows[index].isEnabled && rows[index].bounds.contains(point))
		{
			Select(index);
			return;
		}
	}
}

void OptionsState::HandleMousePress(sf::Vector2i pixelPosition)
{
	HandleMousePosition(pixelPosition);
	if (!IsSelectedRowEnabled())
		return;

	const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pixelPosition) };
	if (!rows[selectedIndex].bounds.contains(point))
		return;

	if (rows[selectedIndex].kind == RowKind::Dropdown &&
		!UI::OptionsWidgets::GetValueBoxBounds(rows[selectedIndex].bounds.position.y).contains(point))
	{
		return;
	}

	if (rows[selectedIndex].kind == RowKind::Slider)
	{
		isSliderDragging = true;
		UpdateSliderFromMouse(point);
	}
	else
	{
		ActivateSelected();
	}
}

void OptionsState::UpdateSliderFromMouse(sf::Vector2f position)
{
	if (!IsSelectedRowEnabled())
		return;
	const Action action{ rows[selectedIndex].action };
	if (action != Action::MusicVolume && action != Action::SoundVolume)
		return;

	const float value{ UI::OptionsWidgets::SliderValueFromMouseX(position.x) };
	GameSettings& settings{ GetContext().settings.EditSettings() };
	if (action == Action::MusicVolume)
		settings.audio.musicVolume = std::round(value);
	else
		settings.audio.soundVolume = std::round(value);
	SaveAndApplyAudio();
	RefreshRowTextValues();
}

void OptionsState::OpenDropdown(Action action)
{
	dropdownAction = action;
	dropdownIndex = action == Action::Resolution
		? FindCurrentResolution()
		: static_cast<std::size_t>(GetContext().settings.GetSettings().graphics.windowMode);
	dropdownFirstVisible = 0u;
	isDropdownOpen = true;
	isDropdownScrollbarDragging = false;
	dropdownLabels.clear();
	dropdownLabels.reserve(GetDropdownItemCount());
	const sf::Font& font{ GetContext().assets.Fonts().Get(
		GetContext().localization.GetRegularFont()) };
	for (std::size_t index{}; index < GetDropdownItemCount(); ++index)
		dropdownLabels.emplace_back(font, GetDropdownItemLabel(index), 24u);
	EnsureDropdownSelectionVisible();
}

void OptionsState::CloseDropdown()
{
	isDropdownOpen = false;
	isDropdownScrollbarDragging = false;
}

void OptionsState::MoveDropdownSelection(int direction)
{
	const std::size_t count{ GetDropdownItemCount() };
	if (count == 0u)
		return;

	const std::size_t previous{ dropdownIndex };
	if (direction < 0 && dropdownIndex > 0u)
		--dropdownIndex;
	else if (direction > 0 && dropdownIndex + 1u < count)
		++dropdownIndex;

	if (dropdownIndex != previous)
	{
		EnsureDropdownSelectionVisible();
		GetContext().audio.PlaySound(
			Config::Sound::ItemSelect,
			SoundGroup::UI,
			100.f,
			1.f,
			SoundPlayback::StopPrevious);
	}
}

void OptionsState::EnsureDropdownSelectionVisible()
{
	const std::size_t count{ GetDropdownItemCount() };
	if (count <= UI::OptionsWidgets::MaximumVisibleDropdownItems)
	{
		dropdownFirstVisible = 0u;
		return;
	}

	if (dropdownIndex < dropdownFirstVisible)
		dropdownFirstVisible = dropdownIndex;
	else if (dropdownIndex >= dropdownFirstVisible + UI::OptionsWidgets::MaximumVisibleDropdownItems)
		dropdownFirstVisible = dropdownIndex - UI::OptionsWidgets::MaximumVisibleDropdownItems + 1u;

	dropdownFirstVisible = std::min(
		dropdownFirstVisible,
		count - UI::OptionsWidgets::MaximumVisibleDropdownItems);
}

void OptionsState::HandleDropdownMouseMove(sf::Vector2f position)
{
	const std::size_t visibleCount{ std::min(UI::OptionsWidgets::MaximumVisibleDropdownItems,
		GetDropdownItemCount() - dropdownFirstVisible) };
	for (std::size_t visibleIndex{ 0u }; visibleIndex < visibleCount; ++visibleIndex)
	{
		if (!GetDropdownItemBounds(visibleIndex).contains(position))
			continue;

		const std::size_t hovered{ dropdownFirstVisible + visibleIndex };
		if (hovered != dropdownIndex)
		{
			dropdownIndex = hovered;
			GetContext().audio.PlaySound(
				Config::Sound::ItemSelect,
				SoundGroup::UI,
				100.f,
				1.f,
				SoundPlayback::StopPrevious);
		}
		return;
	}
}

void OptionsState::HandleDropdownMousePress(sf::Vector2f position)
{
	const std::size_t count{ GetDropdownItemCount() };
	const std::size_t visibleCount{ std::min(UI::OptionsWidgets::MaximumVisibleDropdownItems,
		count - dropdownFirstVisible) };
	if (count > UI::OptionsWidgets::MaximumVisibleDropdownItems && GetDropdownScrollbarBounds().contains(position))
	{
		isDropdownScrollbarDragging = true;
		UpdateDropdownScrollbar(position);
		return;
	}

	for (std::size_t visibleIndex{ 0u }; visibleIndex < visibleCount; ++visibleIndex)
	{
		if (GetDropdownItemBounds(visibleIndex).contains(position))
		{
			dropdownIndex = dropdownFirstVisible + visibleIndex;
			ApplyDropdownSelection();
			return;
		}
	}

	CloseDropdown();
}

void OptionsState::HandleMouseWheel(float delta)
{
	const std::size_t count{ GetDropdownItemCount() };
	if (count <= UI::OptionsWidgets::MaximumVisibleDropdownItems || delta == 0.f)
		return;

	const std::size_t previousSelection{ dropdownIndex };
	const std::size_t maximumFirst{ count - UI::OptionsWidgets::MaximumVisibleDropdownItems };
	if (delta > 0.f && dropdownFirstVisible > 0u)
		--dropdownFirstVisible;
	else if (delta < 0.f && dropdownFirstVisible < maximumFirst)
		++dropdownFirstVisible;

	dropdownIndex = std::clamp(
		dropdownIndex,
		dropdownFirstVisible,
		dropdownFirstVisible + UI::OptionsWidgets::MaximumVisibleDropdownItems - 1u);
	if (dropdownIndex != previousSelection)
	{
		GetContext().audio.PlaySound(
			Config::Sound::ItemSelect,
			SoundGroup::UI,
			100.f,
			1.f,
			SoundPlayback::StopPrevious);
	}
}

void OptionsState::UpdateDropdownScrollbar(sf::Vector2f position)
{
	const std::size_t count{ GetDropdownItemCount() };
	if (count <= UI::OptionsWidgets::MaximumVisibleDropdownItems)
		return;

	const std::size_t previousSelection{ dropdownIndex };
	const sf::FloatRect track{ GetDropdownScrollbarBounds() };
	const float thumbHeight{ track.size.y * static_cast<float>(UI::OptionsWidgets::MaximumVisibleDropdownItems) /
		static_cast<float>(count) };
	const float travel{ track.size.y - thumbHeight };
	const float normalized{ travel <= 0.f
		? 0.f
		: std::clamp((position.y - track.position.y - thumbHeight * 0.5f) / travel, 0.f, 1.f) };
	dropdownFirstVisible = static_cast<std::size_t>(std::round(
		normalized * static_cast<float>(count - UI::OptionsWidgets::MaximumVisibleDropdownItems)));
	dropdownIndex = std::clamp(
		dropdownIndex,
		dropdownFirstVisible,
		dropdownFirstVisible + UI::OptionsWidgets::MaximumVisibleDropdownItems - 1u);
	if (dropdownIndex != previousSelection)
	{
		GetContext().audio.PlaySound(
			Config::Sound::ItemSelect,
			SoundGroup::UI,
			100.f,
			1.f,
			SoundPlayback::StopPrevious);
	}
}

void OptionsState::ApplyDropdownSelection()
{
	GetContext().audio.PlaySound(
		Config::Sound::ItemPress,
		SoundGroup::UI,
		100.f,
		1.f,
		SoundPlayback::StopPrevious);

	if (dropdownAction == Action::Resolution)
		ApplyResolution(dropdownIndex);
	else if (dropdownAction == Action::WindowMode)
		ApplyWindowMode(static_cast<WindowMode>(dropdownIndex));
	else
		CloseDropdown();
}

void OptionsState::Execute(Action action)
{
	switch (action)
	{
	case Action::OpenGraphics:
		BeginPageTransition(Page::Graphics);
		break;
	case Action::OpenAudio:
		BeginPageTransition(Page::Audio);
		break;
	case Action::OpenGameplay:
		BeginPageTransition(Page::Gameplay);
		break;
	case Action::OpenControls:
		BeginPageTransition(Page::Controls);
		break;
	case Action::OpenLanguage:
		BeginPageTransition(Page::Language);
		break;
	case Action::OpenKeyboardControls:
		BeginPageTransition(Page::KeyboardControls);
		break;
	case Action::OpenGamepadControls:
		BeginPageTransition(Page::GamepadControls);
		break;
	case Action::Back:
		if (page == Page::Root)
			BeginExit();
		else if (page == Page::KeyboardControls || page == Page::GamepadControls)
			BeginPageTransition(Page::Controls);
		else
			BeginPageTransition(Page::Root);
		break;
	case Action::ResetAll:
	{
		const GraphicsSettings previous{ GetContext().settings.GetSettings().graphics };
		const LocalizationSettings localization{ GetContext().settings.GetSettings().localization };
		GetContext().settings.EditSettings() = GetContext().settings.GetDefaults();
		GetContext().settings.EditSettings().localization = localization;
		SaveAndApplyAudio();
		if (RequiresWindowRecreation(previous, GetContext().settings.GetSettings().graphics))
			BeginDisplayChange(previous);
		else
			SaveAndApplyLiveGraphics();
		RebuildRows();
		break;
	}
	case Action::ResetGraphics:
	{
		const GraphicsSettings previous{ GetContext().settings.GetSettings().graphics };
		GetContext().settings.EditSettings().graphics = GetContext().settings.GetDefaults().graphics;
		if (RequiresWindowRecreation(previous, GetContext().settings.GetSettings().graphics))
			BeginDisplayChange(previous);
		else
			SaveAndApplyLiveGraphics();
		RebuildRows();
		break;
	}
	case Action::ResetAudio:
		GetContext().settings.EditSettings().audio = GetContext().settings.GetDefaults().audio;
		SaveAndApplyAudio();
		RefreshRowTextValues();
		break;
	case Action::ResetGameplay:
		GetContext().settings.EditSettings().gameplay = GetContext().settings.GetDefaults().gameplay;
		// The gamepad vibration/adaptive-trigger/lightbar toggles live on
		// this page too (see RebuildRows), so resetting it resets those as
		// well rather than leaving them out of "Restore Gameplay Defaults".
		GetContext().settings.EditSettings().gamepad = GetContext().settings.GetDefaults().gamepad;
		SaveSettings();
		RefreshRowTextValues();
		break;
	case Action::ResetControls:
		GetContext().settings.EditSettings().controls = GetContext().settings.GetDefaults().controls;
		SaveSettings();
		RefreshRowTextValues();
		break;
	case Action::SetEnglish:
	case Action::SetSpanish:
	case Action::SetRussian:
	case Action::SetUkrainian:
	case Action::SetArabic:
	{
		Language language{ Language::English };
		if (action == Action::SetSpanish) language = Language::Spanish;
		else if (action == Action::SetRussian) language = Language::Russian;
		else if (action == Action::SetUkrainian) language = Language::Ukrainian;
		else if (action == Action::SetArabic) language = Language::Arabic;
		hasSaveFailed = !GetContext().localization.SetLanguage(language);
		RefreshTitle();
		RebuildRows();
		break;
	}
	default:
		break;
	}
}

void OptionsState::SaveSettings()
{
	hasSaveFailed = !GetContext().settings.SaveSettings();
}

void OptionsState::SaveAndApplyAudio()
{
	SaveSettings();
	GetContext().audio.ApplySettings();
}

void OptionsState::SaveAndApplyLiveGraphics()
{
	SaveSettings();
	GetContext().display.ApplyLiveSettings(GetContext().settings.GetSettings().graphics);
}

void OptionsState::BeginDisplayChange(const GraphicsSettings& previous)
{
	previousGraphics = previous;
	SaveSettings();
	GetContext().display.ApplyDisplaySettings(GetContext().settings.GetSettings().graphics);
	GetContext().window.setMouseCursorVisible(false);
	displayConfirmationRemaining = DisplayConfirmationDuration;
	dialogSelectedIndex = 0u;
	dialogGlow.Invalidate();
	isDisplayConfirmationOpen = true;
}

void OptionsState::SelectDialogOption(std::size_t index, bool playSound)
{
	index = std::min(index, std::size_t{ 1u });
	const bool changed{ dialogSelectedIndex != index };
	dialogSelectedIndex = index;
	if (changed)
		dialogGlow.Invalidate();
	if (changed && playSound)
		GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI);
}

void OptionsState::ActivateDialogOption(std::size_t index)
{
	GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI);
	if (index == 0u)
		ConfirmDisplayChange();
	else
		RevertDisplayChange();
}

void OptionsState::ConfirmDisplayChange()
{
	isDisplayConfirmationOpen = false;
}

void OptionsState::RevertDisplayChange()
{
	GetContext().settings.EditSettings().graphics = previousGraphics;
	SaveSettings();
	GetContext().display.ApplyDisplaySettings(previousGraphics);
	GetContext().window.setMouseCursorVisible(false);
	isDisplayConfirmationOpen = false;
	RebuildRows();
}

void OptionsState::ApplyResolution(std::size_t resolutionIndex)
{
	const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
	if (resolutionIndex >= resolutions.size())
		return;
	const GraphicsSettings previous{ GetContext().settings.GetSettings().graphics };
	if (previous.resolution == resolutions[resolutionIndex])
	{
		CloseDropdown();
		return;
	}
	GetContext().settings.EditSettings().graphics.resolution = resolutions[resolutionIndex];
	CloseDropdown();
	BeginDisplayChange(previous);
}

void OptionsState::ApplyWindowMode(WindowMode mode)
{
	const GraphicsSettings previous{ GetContext().settings.GetSettings().graphics };
	if (previous.windowMode == mode)
	{
		CloseDropdown();
		return;
	}

	GetContext().settings.EditSettings().graphics.windowMode = mode;
	CloseDropdown();
	BeginDisplayChange(previous);
	RebuildRows();
}

bool OptionsState::RequiresWindowRecreation(
	const GraphicsSettings& before,
	const GraphicsSettings& after) const noexcept
{
	if (before.windowMode != after.windowMode)
		return true;

	return before.windowMode != WindowMode::Borderless && before.resolution != after.resolution;
}

void OptionsState::BeginBinding(Action action)
{
	pendingBinding = action;
	dialogGlow.Invalidate();
}

void OptionsState::ApplyBinding(ControlBinding binding)
{
	if (!pendingBinding.has_value())
		return;
	if (ControlBinding* target{ GetBinding(*pendingBinding) })
	{
		const ControlBinding previous{ *target };
		ControlSettings& controls{ GetContext().settings.EditSettings().controls };
		const std::array<ControlBinding*, 5> allBindings{
			&controls.moveUp,
			&controls.moveDown,
			&controls.moveLeft,
			&controls.moveRight,
			&controls.fire
		};
		const auto matches{ [&binding](const ControlBinding& candidate)
			{
				return candidate.device == binding.device && candidate.code == binding.code;
			} };

		for (ControlBinding* existing : allBindings)
		{
			if (existing != target && matches(*existing))
			{
				*existing = previous;
				break;
			}
		}

		*target = binding;
		SaveSettings();
		RefreshRowTextValues();
	}
	pendingBinding.reset();
}

ControlBinding* OptionsState::GetBinding(Action action)
{
	ControlSettings& controls{ GetContext().settings.EditSettings().controls };
	switch (action)
	{
	case Action::MoveUp: return &controls.moveUp;
	case Action::MoveDown: return &controls.moveDown;
	case Action::MoveLeft: return &controls.moveLeft;
	case Action::MoveRight: return &controls.moveRight;
	case Action::Fire: return &controls.fire;
	default: return nullptr;
	}
}

const ControlBinding* OptionsState::GetBinding(Action action) const
{
	const ControlSettings& controls{ GetContext().settings.GetSettings().controls };
	switch (action)
	{
	case Action::MoveUp: return &controls.moveUp;
	case Action::MoveDown: return &controls.moveDown;
	case Action::MoveLeft: return &controls.moveLeft;
	case Action::MoveRight: return &controls.moveRight;
	case Action::Fire: return &controls.fire;
	default: return nullptr;
	}
}

sf::String OptionsState::GetRowValue(const Row& row) const
{
	const GameSettings& settings{ GetContext().settings.GetSettings() };
	switch (row.action)
	{
	case Action::Resolution:
		if (!row.isEnabled)
			return GetContext().localization.GetText("options.desktop_resolution");
		return std::to_string(settings.graphics.resolution.x) + " x " +
			std::to_string(settings.graphics.resolution.y);
	case Action::WindowMode:
		switch (settings.graphics.windowMode)
		{
		case WindowMode::Fullscreen: return GetContext().localization.GetText("options.fullscreen");
		case WindowMode::Windowed: return GetContext().localization.GetText("options.windowed");
		case WindowMode::Borderless: return GetContext().localization.GetText("options.borderless");
		}
		return GetContext().localization.GetText("common.unknown");
	case Action::ShowFps:
		return GetContext().localization.GetText(settings.graphics.needToShowFPS ? "common.on" : "common.off");
	case Action::VerticalSync:
		return GetContext().localization.GetText(settings.graphics.isVSyncEnabled ? "common.on" : "common.off");
	case Action::PostEffects:
		return GetContext().localization.GetText(settings.graphics.arePostEffectsEnabled ? "common.on" : "common.off");
	case Action::ScreenShake:
		return GetContext().localization.GetText(settings.gameplay.isScreenShakeEnabled ? "common.on" : "common.off");
	case Action::ShowScorePopups:
		return GetContext().localization.GetText(settings.gameplay.needToShowScorePopups ? "common.on" : "common.off");
	case Action::GamepadVibration:
		return GetContext().localization.GetText(settings.gamepad.isVibrationEnabled ? "common.on" : "common.off");
	case Action::GamepadAdaptiveTriggers:
		return GetContext().localization.GetText(settings.gamepad.isAdaptiveTriggersEnabled ? "common.on" : "common.off");
	case Action::GamepadLightbar:
		return GetContext().localization.GetText(settings.gamepad.isControllerLightbarEnabled ? "common.on" : "common.off");
	case Action::FrameRateLimit:
		return settings.graphics.frameRateLimit == 0u
			? GetContext().localization.GetText("options.unlimited")
			: sf::String(std::to_string(settings.graphics.frameRateLimit));
	case Action::MusicVolume:
		return std::to_string(static_cast<int>(std::round(settings.audio.musicVolume))) + "%";
	case Action::SoundVolume:
		return std::to_string(static_cast<int>(std::round(settings.audio.soundVolume))) + "%";
	default:
		if (const ControlBinding* binding{ GetBinding(row.action) })
			return GetBindingName(*binding);
		return {};
	}
}

sf::String OptionsState::GetBindingName(const ControlBinding& binding) const
{
	if (binding.device == RebindableInputDevice::Mouse)
	{
		switch (static_cast<sf::Mouse::Button>(binding.code))
		{
		case sf::Mouse::Button::Left: return GetContext().localization.GetText("options.mouse_left");
		case sf::Mouse::Button::Right: return GetContext().localization.GetText("options.mouse_right");
		case sf::Mouse::Button::Middle: return GetContext().localization.GetText("options.mouse_middle");
		case sf::Mouse::Button::Extra1: return GetContext().localization.GetText("options.mouse_4");
		case sf::Mouse::Button::Extra2: return GetContext().localization.GetText("options.mouse_5");
		}
		return GetContext().localization.GetText("options.mouse");
	}

	const auto key{ static_cast<sf::Keyboard::Key>(binding.code) };
	return sf::Keyboard::getDescription(sf::Keyboard::delocalize(key));
}

std::size_t OptionsState::FindCurrentResolution() const
{
	const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
	const auto iterator{ std::ranges::find(resolutions,
		GetContext().settings.GetSettings().graphics.resolution) };
	return iterator == resolutions.end()
		? 0u
		: static_cast<std::size_t>(std::distance(resolutions.begin(), iterator));
}

std::size_t OptionsState::GetDropdownItemCount() const
{
	if (dropdownAction == Action::Resolution)
		return GetContext().display.GetSupportedResolutions().size();
	if (dropdownAction == Action::WindowMode)
		return 3u;
	return 0u;
}

sf::String OptionsState::GetDropdownItemLabel(std::size_t index) const
{
	if (dropdownAction == Action::Resolution)
	{
		const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
		if (index < resolutions.size())
			return std::to_string(resolutions[index].x) + " x " +
				std::to_string(resolutions[index].y);
	}
	else if (dropdownAction == Action::WindowMode && index < 3u)
	{
		const std::array keys{ "options.fullscreen", "options.windowed", "options.borderless" };
		return GetContext().localization.GetText(keys[index]);
	}
	return {};
}

sf::FloatRect OptionsState::GetDropdownItemBounds(std::size_t visibleIndex) const
{
	const auto activeRow{ std::ranges::find_if(rows, [this](const Row& row)
		{
			return row.action == dropdownAction;
		}) };
	if (activeRow == rows.end())
		return {};

	const sf::FloatRect valueBox{ UI::OptionsWidgets::GetValueBoxBounds(activeRow->bounds.position.y) };
	return UI::OptionsWidgets::GetDropdownItemBounds(valueBox, GetDropdownItemCount(), visibleIndex);
}

sf::FloatRect OptionsState::GetDropdownScrollbarBounds() const
{
	const auto activeRow{ std::ranges::find_if(rows, [this](const Row& row)
		{
			return row.action == dropdownAction;
		}) };
	if (activeRow == rows.end())
		return {};

	const sf::FloatRect valueBox{ UI::OptionsWidgets::GetValueBoxBounds(activeRow->bounds.position.y) };
	return UI::OptionsWidgets::GetDropdownScrollbarBounds(valueBox, GetDropdownItemCount());
}

bool OptionsState::IsSelectedRowEnabled() const
{
	return selectedIndex < rows.size() && rows[selectedIndex].isEnabled;
}

void OptionsState::DrawTitle(sf::RenderTarget& target)
{
	titleGlow.DrawBloom(target, title.getGlobalBounds(),
		[this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
		{ glowTarget.draw(title, states); }, Cyan);
	target.draw(title);
}

void OptionsState::DrawGamepadLayouts(sf::RenderTarget& target)
{
	if (isGamepadLayoutCacheDirty)
	{
		const sf::Vector2u size{
			static_cast<unsigned int>(GetContext().logicalSize.x),
			static_cast<unsigned int>(GetContext().logicalSize.y) };
		if (gamepadLayoutCache.getSize() != size && !gamepadLayoutCache.resize(size))
			return;
		gamepadLayoutCache.clear(sf::Color::Transparent);
		DrawGamepadLayoutsContent(gamepadLayoutCache);
		gamepadLayoutCache.display();
		isGamepadLayoutCacheDirty = false;
	}
	target.draw(sf::Sprite(gamepadLayoutCache.getTexture()));
}

void OptionsState::DrawGamepadLayoutsContent(sf::RenderTarget& target)
{
	const auto drawPanel{ [this, &target](
		const sf::String& heading,
		float top,
		const std::array<Config::Texture, 7>& icons,
		const std::array<sf::String, 7>& actions)
	{
		UI::RoundedRectangleShape panel({ 1400.f, 310.f }, 18.f, 12u);
		panel.setPosition({ 260.f, top });
		panel.setFillColor(sf::Color(3, 15, 28, 235));
		panel.setOutlineColor(sf::Color(40, 132, 160, 205));
		panel.setOutlineThickness(2.f);
		target.draw(panel);

		sf::Text headingText(
			GetContext().assets.Fonts().Get(GetContext().localization.GetBoldFont()), heading, 34u);
		const sf::FloatRect headingLocalBounds{ headingText.getLocalBounds() };
		headingText.setOrigin({
			headingLocalBounds.position.x + headingLocalBounds.size.x * 0.5f,
			headingLocalBounds.position.y + headingLocalBounds.size.y * 0.5f });
		headingText.setPosition({ 960.f, top + 37.f });
		headingText.setFillColor(BrightCyan);
		headingText.setOutlineColor(sf::Color(3, 18, 31, 240));
		headingText.setOutlineThickness(2.f);
		target.draw(headingText);

		sf::RectangleShape dividerGlow({ 1300.f, 7.f });
		dividerGlow.setPosition({ 310.f, top + 71.f });
		dividerGlow.setFillColor(sf::Color(255, 166, 42, 38));
		target.draw(dividerGlow);
		sf::RectangleShape divider({ 1300.f, 2.f });
		divider.setPosition({ 310.f, top + 73.f });
		divider.setFillColor(Orange);
		target.draw(divider);

		for (std::size_t index{ 0u }; index < icons.size(); ++index)
		{
			const bool rightColumn{ index >= 4u };
			const std::size_t row{ rightColumn ? index - 4u : index };
			const sf::Vector2f cardPosition{
				rightColumn ? 980.f : 300.f,
				top + 86.f + static_cast<float>(row) * 51.f };
			constexpr sf::Vector2f CardSize{ 640.f, 46.f };

			UI::RoundedRectangleShape card(CardSize, 10.f, 8u);
			card.setPosition(cardPosition);
			card.setFillColor(sf::Color(5, 27, 43, 218));
			card.setOutlineColor(sf::Color(45, 126, 151, 180));
			card.setOutlineThickness(1.f);
			target.draw(card);

			const sf::Texture& texture{ GetContext().assets.Textures().Get(icons[index]) };
			sf::Sprite icon(texture);
			const sf::Vector2u size{ texture.getSize() };
			const float scale{ std::min(
				70.f / static_cast<float>(size.x),
				42.f / static_cast<float>(size.y)) };
			icon.setOrigin({
				static_cast<float>(size.x) * 0.5f,
				static_cast<float>(size.y) * 0.5f });
			icon.setScale({ scale, scale });
			icon.setPosition({ cardPosition.x + 48.f, cardPosition.y + CardSize.y * 0.5f });
			target.draw(icon);

			UI::OptionsWidgets::DrawText(target,
				GetContext().assets.Fonts().Get(GetContext().localization.GetRegularFont()),
				actions[index],
				{ cardPosition.x + 100.f, cardPosition.y + 7.f }, 25u, BrightCyan);
		}
	} };

	const std::array<Config::Texture, 7> xboxIcons{
		Config::Texture::XboxLeftStick,
		Config::Texture::XboxRightStick,
		Config::Texture::XboxRightTrigger,
		Config::Texture::XboxDpad,
		Config::Texture::XboxConfirm,
		Config::Texture::XboxBack,
		Config::Texture::XboxMenu
	};
	const std::array<Config::Texture, 7> playStationIcons{
		Config::Texture::PlayStationLeftStick,
		Config::Texture::PlayStationRightStick,
		Config::Texture::PlayStationRightTrigger,
		Config::Texture::PlayStationDpad,
		Config::Texture::PlayStationConfirm,
		Config::Texture::PlayStationBack,
		Config::Texture::PlayStationOptions
	};
	const std::array<sf::String, 7> actions{
		GetContext().localization.GetText("options.move"), GetContext().localization.GetText("options.aim"),
		GetContext().localization.GetText("options.fire"), GetContext().localization.GetText("options.menu_navigation"),
		GetContext().localization.GetText("common.confirm"), GetContext().localization.GetText("options.back"),
		GetContext().localization.GetText("pause.title")
	};

	drawPanel(GetContext().localization.GetText("options.xbox_controller"), 190.f, xboxIcons, actions);
	drawPanel(GetContext().localization.GetText("options.playstation_controller"), 515.f, playStationIcons, actions);
}

void OptionsState::DrawRows(sf::RenderTarget& target)
{
	if (!isDisplayConfirmationOpen && !pendingBinding.has_value() && IsSelectedRowEnabled())
	{
		const Row& selectedRow{ rows[selectedIndex] };
		neonGlow.DrawBloom(
			target,
			selectedRow.bounds,
			[this, &selectedRow](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				DrawRow(glowTarget, selectedRow, selectedIndex, states);
			},
			UI::MenuTheme::SelectionGlow);
	}

	for (std::size_t index{ 0u }; index < rows.size(); ++index)
		DrawRow(target, rows[index], index, sf::RenderStates::Default);

	if (!isDisplayConfirmationOpen && !pendingBinding.has_value() && IsSelectedRowEnabled())
		neonGlow.DrawHighlight(target, rows[selectedIndex].bounds, UI::MenuTheme::SelectionGlow);

	if (hasSaveFailed)
	{
		UI::OptionsWidgets::DrawText(target,
			GetContext().assets.Fonts().Get(GetContext().localization.GetRegularFont()),
			GetContext().localization.GetText("options.save_error"),
			{ 570.f, 930.f }, 23, Red);
	}
}

void OptionsState::DrawRow(
	sf::RenderTarget& target,
	const Row& row,
	std::size_t index,
	const sf::RenderStates& states)
{
	const bool selected{ index == selectedIndex && row.isEnabled };
	UI::RoundedRectangleShape panel(row.bounds.size, 15.f, 10u);
	panel.setPosition(row.bounds.position);
	panel.setFillColor(selected ? sf::Color(8, 34, 48, 226) : sf::Color(5, 17, 29, 210));
	panel.setOutlineColor(selected ? Cyan : sf::Color(52, 76, 92));
	panel.setOutlineThickness(selected ? 2.f : 1.f);
	target.draw(panel, states);

	const sf::Color textColor{ !row.isEnabled ? Disabled : (selected ? Orange : BrightCyan) };
	rowLabels[index].setFillColor(textColor);
	target.draw(rowLabels[index], states);

	if (row.kind == RowKind::Slider)
	{
		const float value{ row.action == Action::MusicVolume
			? GetContext().settings.GetSettings().audio.musicVolume
			: GetContext().settings.GetSettings().audio.soundVolume };
		UI::OptionsWidgets::DrawSlider(target, row.bounds.position.y, value, states);
		rowValues[index].setFillColor(Cyan);
		target.draw(rowValues[index], states);
	}
	else if (row.kind == RowKind::Toggle)
	{
		bool value{ false };
		if (row.action == Action::ShowFps)
			value = GetContext().settings.GetSettings().graphics.needToShowFPS;
		else if (row.action == Action::VerticalSync)
			value = GetContext().settings.GetSettings().graphics.isVSyncEnabled;
		else if (row.action == Action::PostEffects)
			value = GetContext().settings.GetSettings().graphics.arePostEffectsEnabled;
		else if (row.action == Action::ScreenShake)
			value = GetContext().settings.GetSettings().gameplay.isScreenShakeEnabled;
		else if (row.action == Action::ShowScorePopups)
			value = GetContext().settings.GetSettings().gameplay.needToShowScorePopups;
		else if (row.action == Action::GamepadVibration)
			value = GetContext().settings.GetSettings().gamepad.isVibrationEnabled;
		else if (row.action == Action::GamepadAdaptiveTriggers)
			value = GetContext().settings.GetSettings().gamepad.isAdaptiveTriggersEnabled;
		else if (row.action == Action::GamepadLightbar)
			value = GetContext().settings.GetSettings().gamepad.isControllerLightbarEnabled;
		UI::OptionsWidgets::DrawToggle(target, row.bounds.position.y, value, toggleOnText, toggleOffText, states);
	}
	else if (row.kind == RowKind::Dropdown)
	{
		const sf::FloatRect bounds{ UI::OptionsWidgets::GetValueBoxBounds(row.bounds.position.y) };
		UI::OptionsWidgets::DrawDropdownBox(target, bounds, row.isEnabled, states);

		rowValues[index].setFillColor(row.isEnabled ? Cyan : Disabled);
		target.draw(rowValues[index], states);
		if (!row.isEnabled)
			target.draw(rowHints[index], states);
	}
	else
	{
		if (!rowValues[index].getString().isEmpty())
		{
			rowValues[index].setFillColor(row.isEnabled ? Cyan : Disabled);
			target.draw(rowValues[index], states);
		}
	}
}

void OptionsState::DrawDropdown(sf::RenderTarget& target)
{
	const std::size_t itemCount{ GetDropdownItemCount() };
	if (itemCount == 0u)
		return;

	const std::size_t visibleCount{ std::min(
		UI::OptionsWidgets::MaximumVisibleDropdownItems,
		itemCount - dropdownFirstVisible) };
	const std::size_t selectedVisibleIndex{ dropdownIndex - dropdownFirstVisible };
	const sf::FloatRect selectedBounds{ GetDropdownItemBounds(selectedVisibleIndex) };
	for (std::size_t visibleIndex{ 0u }; visibleIndex < visibleCount; ++visibleIndex)
	{
		const std::size_t itemIndex{ dropdownFirstVisible + visibleIndex };
		const sf::FloatRect bounds{ GetDropdownItemBounds(visibleIndex) };
		UI::OptionsWidgets::DrawDropdownItem(
			target,
			bounds,
			dropdownLabels[itemIndex],
			itemIndex == dropdownIndex,
			sf::RenderStates::Default);
	}

	if (itemCount > UI::OptionsWidgets::MaximumVisibleDropdownItems)
	{
		const sf::FloatRect trackBounds{ GetDropdownScrollbarBounds() };
		UI::RoundedRectangleShape track(trackBounds.size, 5.f, 6u);
		track.setPosition(trackBounds.position);
		track.setFillColor(sf::Color(22, 42, 55, 235));
		target.draw(track);

		const float thumbHeight{ trackBounds.size.y *
			static_cast<float>(UI::OptionsWidgets::MaximumVisibleDropdownItems) / static_cast<float>(itemCount) };
		const float progress{ static_cast<float>(dropdownFirstVisible) /
			static_cast<float>(itemCount - UI::OptionsWidgets::MaximumVisibleDropdownItems) };
		UI::RoundedRectangleShape thumb({ trackBounds.size.x, thumbHeight }, 5.f, 6u);
		thumb.setPosition({ trackBounds.position.x,
			trackBounds.position.y + (trackBounds.size.y - thumbHeight) * progress });
		thumb.setFillColor(Cyan);
		target.draw(thumb);
	}

}

void OptionsState::DrawDialog(sf::RenderTarget& target)
{
	const sf::Font& font{ GetContext().assets.Fonts().Get(GetContext().localization.GetRegularFont()) };

	sf::RectangleShape veil(GetContext().logicalSize);
	veil.setFillColor(sf::Color(0, 2, 6, 190));
	target.draw(veil);
	UI::RoundedRectangleShape dialog({ 900.f, 260.f }, 22.f, 12u);
	dialog.setPosition({ 510.f, 410.f });
	dialog.setFillColor(sf::Color(4, 19, 31, 248));
	dialog.setOutlineColor(Cyan);
	dialog.setOutlineThickness(2.f);
	target.draw(dialog);

	if (isDisplayConfirmationOpen)
	{
		UI::OptionsWidgets::DrawCenteredText(target, font, GetContext().localization.GetText("options.keep_display"), 960.f, 465.f, 36, BrightCyan);
		UI::OptionsWidgets::DrawCenteredText(target, font, GetContext().localization.FormatText("options.reverting", "seconds",
			std::to_string(static_cast<int>(std::ceil(displayConfirmationRemaining)))),
			960.f, 520.f, 24, Orange);

		const std::array<std::pair<sf::FloatRect, sf::String>, 2> buttons{
			std::pair{ DialogConfirmBounds, GetContext().localization.GetText("common.confirm") },
			std::pair{ DialogCancelBounds, GetContext().localization.GetText("common.cancel") }
		};
		const auto& [selectedBounds, selectedLabel]{ buttons[dialogSelectedIndex] };
		dialogGlow.DrawBloom(
			target,
			selectedBounds,
			[this, &font, &selectedBounds, &selectedLabel](
				sf::RenderTarget& glowTarget,
				const sf::RenderStates& states)
			{
				UI::OptionsWidgets::DrawDialogButton(
					glowTarget,
					font,
					selectedBounds,
					selectedLabel,
					true,
					states);
			},
			UI::MenuTheme::SelectionGlow);
		for (std::size_t index{ 0u }; index < buttons.size(); ++index)
		{
			const auto& [bounds, label]{ buttons[index] };
			UI::OptionsWidgets::DrawDialogButton(
				target,
				font,
				bounds,
				label,
				index == dialogSelectedIndex,
				sf::RenderStates::Default);
		}
		dialogGlow.DrawHighlight(target, selectedBounds, UI::MenuTheme::SelectionGlow);
	}
	else
	{
		UI::OptionsWidgets::DrawCenteredText(target, font, GetContext().localization.GetText("options.press_binding"), 960.f, 475.f, 34, BrightCyan);
		const sf::String cancelLabel{ GetContext().localization.GetText("common.cancel") };
		dialogGlow.DrawBloom(
			target,
			BindingCancelBounds,
			[this, &font, &cancelLabel](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
			{
				UI::OptionsWidgets::DrawDialogButton(
					glowTarget,
					font,
					BindingCancelBounds,
					cancelLabel,
					true,
					states);
			},
			UI::MenuTheme::SelectionGlow);
		UI::OptionsWidgets::DrawDialogButton(
			target,
			font,
			BindingCancelBounds,
			cancelLabel,
			true,
			sf::RenderStates::Default);
		dialogGlow.DrawHighlight(target, BindingCancelBounds, UI::MenuTheme::SelectionGlow);
	}
}
