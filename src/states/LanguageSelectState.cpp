#include "LanguageSelectState.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "states/StateID.h"
#include "input/gamepad/GamepadManager.h"
#include "ui/MenuTheme.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr sf::Vector2f ButtonSize{ 620.f, 86.f };
	constexpr sf::Vector2f FirstButton{ 650.f, 340.f };
	constexpr float Spacing{ 104.f };
	constexpr float FadeDuration{ 0.35f };
}

LanguageSelectState::LanguageSelectState(StateStack& stack, StateContext context)
	: MenuState(stack, context)
	, glow(context.assets)
	, buttonGlow(context.assets)
	, title(context.assets.Fonts().Get(Config::Font::LocalizedBold), "SELECT LANGUAGE", 68u)
	, hint(context.assets.Fonts().Get(Config::Font::LocalizedRegular),
		"You can change the language later in Options", 25u)
{
	context.window.setMouseCursorVisible(false);
	title.setFillColor({ 215, 247, 252 });
	title.setOutlineColor({ 3, 18, 31, 235 });
	title.setOutlineThickness(3.f);
	hint.setFillColor({ 155, 205, 215 });

	const auto centerText{ [this](sf::Text& text, float y)
		{
			const auto bounds{ text.getLocalBounds() };
			text.setOrigin(bounds.position + bounds.size * 0.5f);
			text.setPosition({ GetContext().logicalSize.x * 0.5f, y });
		} };
	centerText(title, 190.f);
	centerText(hint, 270.f);

	const auto& idle{ context.assets.Textures().Get(Config::Texture::MenuButtonIdle) };
	const auto& active{ context.assets.Textures().Get(Config::Texture::MenuButtonSelected) };
	buttons.reserve(Languages.size());
	for (std::size_t i{}; i < Languages.size(); ++i)
	{
		const bool arabic{ Languages[i] == Language::Arabic };
		const auto fontID{ arabic ? Config::Font::ArabicRegular : Config::Font::LocalizedRegular };
		buttons.emplace_back(context.assets.Fonts().Get(fontID), idle, active, "", ButtonSize);
		buttons.back().SetLabel(LocalizationManager::GetLanguageNativeName(Languages[i]));
		buttons.back().SetPosition(FirstButton + sf::Vector2f{ 0.f, Spacing * static_cast<float>(i) });
	}
	Select(0u, false);
	Chrome().StartFadeIn(FadeDuration);
}

void LanguageSelectState::HandleEvent(const sf::Event& event)
{
	// Hover feedback remains available during the short entrance fade. Only a
	// confirmation is delayed, preventing the event that skipped the splash
	// from immediately choosing a language.
	using enum GamepadManager::NavigationAction;
	const auto navigation{ GetContext().gamepad.GetNavigationAction(event) };
	if (!Chrome().IsFading()) switch (navigation)
	{
	case Up: Select((selectedIndex + buttons.size() - 1u) % buttons.size()); return;
	case Down: Select((selectedIndex + 1u) % buttons.size()); return;
	case Confirm: ConfirmSelection(); return;
	default: break;
	}

	if (!Chrome().IsFading())
	{
		if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
		{
			switch (key->code)
			{
			case sf::Keyboard::Key::Up:
			case sf::Keyboard::Key::W:
				Select((selectedIndex + buttons.size() - 1u) % buttons.size()); return;
			case sf::Keyboard::Key::Down:
			case sf::Keyboard::Key::S:
				Select((selectedIndex + 1u) % buttons.size()); return;
			case sf::Keyboard::Key::Enter:
			case sf::Keyboard::Key::Space:
				ConfirmSelection(); return;
			default: break;
			}
		}
	}

	if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
		Chrome().SetMousePosition(point);
		for (std::size_t i{}; i < buttons.size(); ++i)
			if (buttons[i].Contains(point)) { Select(i); break; }
	}
	if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() })
	{
		if (Chrome().IsFading() || pressed->button != sf::Mouse::Button::Left) return;
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pressed->position) };
		for (std::size_t i{}; i < buttons.size(); ++i)
		{
			if (!buttons[i].Contains(point)) continue;
			Select(i, false);
			ConfirmSelection();
			return;
		}
	}
}

void LanguageSelectState::OnUpdate(float deltaTime)
{
	glow.Update(deltaTime);
	buttonGlow.Update(deltaTime);
}

void LanguageSelectState::OnRender()
{
	auto& window{ GetContext().window };
	glow.DrawBloom(window, title.getGlobalBounds(),
		[this](sf::RenderTarget& target, const sf::RenderStates& states)
		{
			target.draw(title, states);
		}, UI::MenuTheme::InterfaceGlow);
	window.draw(title);
	window.draw(hint);
	if (!buttons.empty())
	{
		const UI::MenuButton& selectedButton{ buttons[selectedIndex] };
		buttonGlow.DrawBloom(window, selectedButton.GetBounds(),
			[&selectedButton](sf::RenderTarget& target, const sf::RenderStates& states)
			{ selectedButton.Draw(target, states); }, UI::MenuTheme::SelectionGlow);
	}
	for (const auto& button : buttons) button.Draw(window);
	if (!buttons.empty())
		buttonGlow.DrawHighlight(window, buttons[selectedIndex].GetBounds(), UI::MenuTheme::SelectionGlow);
}

void LanguageSelectState::RenderOverlay()
{
	auto& window{ GetContext().window };
	// Unlike the other menus, this one shows the cursor even under a gamepad,
	// parked beside the selected language, so first-run players see where
	// "click" would land.
	if (GetContext().gamepad.IsInUse())
		Chrome().Cursor().DrawAt(window, cursorPosition);
	else
		Chrome().Cursor().Draw(window);
	Chrome().Fade().Draw(window);
}

void LanguageSelectState::Select(std::size_t index, bool playSound)
{
	if (index >= buttons.size()) return;
	const bool changed{ selectedIndex != index };
	selectedIndex = index;
	for (std::size_t i{}; i < buttons.size(); ++i) buttons[i].SetSelected(i == selectedIndex);
	const auto bounds{ buttons[selectedIndex].GetBounds() };
	cursorPosition = { bounds.position.x - 50.f, bounds.position.y + bounds.size.y * 0.5f };
	if (changed)
		buttonGlow.Invalidate();
	if (playSound && changed)
		GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI);
}

void LanguageSelectState::ConfirmSelection()
{
	if (!GetContext().localization.SetLanguage(Languages[selectedIndex])) return;
	PlayPressSound();
	RequestClear();
	RequestPush(StateID::MainMenu);
}
