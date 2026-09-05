#include "CreditsState.h"

#include <array>
#include <string>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include "assets/Assets.h"
#include "localization/LocalizationManager.h"
#include "input/gamepad/GamepadManager.h"
#include "ui/MenuTheme.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr sf::FloatRect PanelBounds{ { 160.f, 130.f }, { 1600.f, 750.f } };
	constexpr sf::Vector2f ButtonSize{ 540.f, 92.f };
	constexpr float FadeDuration{ 0.35f };
	constexpr unsigned int BodyAtlasSize{ 34u };

	struct CreditLine
	{
		const char* key;
		unsigned int size;
		sf::Color color;
		float y;
	};

	const std::array<CreditLine, 11> CreditLines{
		CreditLine{ "credits.line_1", 34u, UI::MenuTheme::SelectionGlow, 250.f }, CreditLine{ "credits.line_2", 28u, sf::Color(215, 238, 244), 310.f },
		CreditLine{ "credits.line_3", 27u, sf::Color(215, 238, 244), 360.f }, CreditLine{ "credits.line_4", 27u, sf::Color(215, 238, 244), 406.f },
		CreditLine{ "credits.line_5", 27u, sf::Color(215, 238, 244), 450.f }, CreditLine{ "credits.line_6", 29u, sf::Color(235, 246, 249), 506.f },
		CreditLine{ "credits.contact", 25u, sf::Color(150, 215, 230), 582.f }, CreditLine{ "credits.email", 28u, UI::MenuTheme::InterfaceGlow, 622.f },
		CreditLine{ "credits.youtube", 25u, sf::Color(150, 215, 230), 680.f }, CreditLine{ "credits.source", 24u, sf::Color(150, 215, 230), 738.f },
		CreditLine{ "credits.repository", 25u, UI::MenuTheme::InterfaceGlow, 776.f }
	};
}

CreditsState::CreditsState(StateStack& stack, StateContext context)
	: MenuState(stack, context)
	, panel(context.assets.Textures().Get(Config::Texture::CampaignCompletePanelFrame),
		PanelBounds, 190u, { 90.f, 90.f })
	, titleGlow(context.assets)
	, buttonGlow(context.assets)
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()), context.localization.GetText("credits.title"), 72u)
	, returnButton(context.assets.Fonts().Get(context.localization.GetRegularFont()),
		context.assets.Textures().Get(Config::Texture::MenuButtonIdle),
		context.assets.Textures().Get(Config::Texture::MenuButtonSelected),
		context.localization.GetText("common.back_main"), ButtonSize)
{
	context.window.setMouseCursorVisible(false);
	title.setFillColor({ 215, 247, 252 });
	title.setOutlineColor({ 3, 18, 31, 235 });
	title.setOutlineThickness(3.5f);
	UI::TextLayout::CenterText(title, { 960.f, 82.f });

	bodyLines.reserve(CreditLines.size());
	const sf::Font& bodyFont{ context.assets.Fonts().Get(context.localization.GetRegularFont(false)) };
	for (const CreditLine& line : CreditLines)
	{
		bodyLines.emplace_back(bodyFont, context.localization.GetText(line.key), BodyAtlasSize);
		bodyLines.back().setFillColor(line.color);
		bodyLines.back().setOutlineColor({ 0, 8, 15, 225 });
		bodyLines.back().setOutlineThickness(1.25f);
	}
	RefreshLocalizedContent();

	returnButton.SetPosition({ 690.f, 900.f });
	returnButton.SetSelected(false);
	returnButton.SetLabelOutline({ 45, 18, 0, 220 }, 1.5f);
	Chrome().StartFadeIn(FadeDuration);
}

void CreditsState::HandleEvent(const sf::Event& event)
{
	if (IsTransitioning() || Chrome().IsFading()) return;
	using enum GamepadManager::NavigationAction;
	const auto navigation{ GetContext().gamepad.GetNavigationAction(event) };
	if (navigation == Confirm || navigation == Back) { BeginReturn(); return; }
	if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
		Chrome().SetMousePosition(point);
		const bool wasSelected{ isReturnButtonSelected };
		isReturnButtonSelected = returnButton.Contains(point);
		returnButton.SetSelected(isReturnButtonSelected);
		if (isReturnButtonSelected != wasSelected)
			buttonGlow.Invalidate();
		return;
	}
	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
	{
		if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space ||
			key->code == sf::Keyboard::Key::Escape) BeginReturn();
		return;
	}
	if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() })
		if (pressed->button == sf::Mouse::Button::Left && returnButton.Contains(
			GetContext().window.mapPixelToCoords(pressed->position))) BeginReturn();
}

void CreditsState::OnUpdate(float deltaTime)
{
	titleGlow.Update(deltaTime);
	buttonGlow.Update(deltaTime);
	if (GetContext().gamepad.IsInUse() && !isReturnButtonSelected)
	{
		isReturnButtonSelected = true;
		returnButton.SetSelected(true);
		buttonGlow.Invalidate();
	}
}

void CreditsState::OnRender()
{
	auto& window{ GetContext().window };
	panel.Draw(window);
	titleGlow.DrawBloom(window, title.getGlobalBounds(),
		[this](sf::RenderTarget& target, const sf::RenderStates& states)
		{ target.draw(title, states); }, UI::MenuTheme::InterfaceGlow);
	window.draw(title);
	for (const sf::Text& line : bodyLines) window.draw(line);
	if (isReturnButtonSelected)
		buttonGlow.DrawBloom(window, returnButton.GetBounds(),
			[this](sf::RenderTarget& target, const sf::RenderStates& states)
			{ returnButton.Draw(target, states); }, UI::MenuTheme::SelectionGlow);
	returnButton.Draw(window);
	if (isReturnButtonSelected)
		buttonGlow.DrawHighlight(window, returnButton.GetBounds(), UI::MenuTheme::SelectionGlow);
}

void CreditsState::OnReactivated()
{
	GetContext().window.setMouseCursorVisible(false);
	if (localizationRevision != GetContext().localization.GetLanguageRevision())
		RefreshLocalizedContent();
	ResetTransition();
	isReturnButtonSelected = false;
	returnButton.SetSelected(false);
	buttonGlow.Invalidate();
	Chrome().StartFadeIn(FadeDuration);
}

void CreditsState::RefreshLocalizedContent()
{
	const StateContext& context{ GetContext() };
	localizationRevision = context.localization.GetLanguageRevision();

	title.setFont(context.assets.Fonts().Get(context.localization.GetBoldFont()));
	title.setString(context.localization.GetText("credits.title"));
	UI::TextLayout::CenterText(title, { 960.f, 82.f });
	titleGlow.Invalidate();

	const sf::Font& bodyFont{ context.assets.Fonts().Get(context.localization.GetRegularFont(false)) };
	for (std::size_t index{ 0u }; index < CreditLines.size(); ++index)
	{
		const CreditLine& line{ CreditLines[index] };
		sf::Text& text{ bodyLines[index] };
		text.setFont(bodyFont);
		text.setString(context.localization.GetText(line.key));
		const float visualScale{ static_cast<float>(line.size) / static_cast<float>(BodyAtlasSize) };
		text.setScale({ visualScale, visualScale });
		UI::TextLayout::FitWidth(text, PanelBounds.size.x - 180.f, 18u);
		UI::TextLayout::CenterText(text, { 960.f, line.y });
	}

	returnButton.SetFont(context.assets.Fonts().Get(context.localization.GetRegularFont()));
	returnButton.SetLabel(context.localization.GetText("common.back_main"));
}

void CreditsState::BeginReturn()
{
	if (IsTransitioning()) return;
	PlayPressSound();
	BeginTransition(FadeDuration, [this] { RequestPop(); });
}
