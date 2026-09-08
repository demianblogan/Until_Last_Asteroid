#include "CampaignCompleteState.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "states/StateID.h"
#include "input/gamepad/GamepadManager.h"
#include "ui/TextLayout.h"
#include "utils/ConfigEnums.h"

namespace
{
	constexpr sf::FloatRect TitleBounds{ { 210.f, 30.f }, { 1500.f, 260.f } };
	constexpr sf::FloatRect MessageBounds{ { 80.f, 285.f }, { 1760.f, 590.f } };
	constexpr float TitleBorder = 85.f;
	constexpr float MessageBorder = 100.f;
	constexpr sf::Vector2f ButtonSize{ 500.f, 98.f };
	constexpr sf::Vector2f ButtonPosition{ 710.f, 888.f };
	constexpr sf::Color Gold{ 255, 188, 62 };
	constexpr float RevealDuration = 0.9f;
	constexpr float FadeDuration = 0.55f;
}

CampaignCompleteState::CampaignCompleteState(StateStack& stack, StateContext context)
	: MenuState(stack, context, Gold)
	, titleFrame(context.assets.Textures().Get(Config::Texture::CampaignCompleteTitleFrame),
		TitleBounds, 220u, { TitleBorder, TitleBorder })
	, messageFrame(context.assets.Textures().Get(Config::Texture::CampaignCompletePanelFrame),
		MessageBounds, 190u, { MessageBorder, MessageBorder })
	, titleGlow(context.assets)
	, buttonGlow(context.assets)
	, title(context.assets.Fonts().Get(context.localization.GetBoldFont()),
		context.localization.GetText("campaign_complete.title"), 60u)
	, button(context.assets.Fonts().Get(context.localization.GetRegularFont()),
		context.assets.Textures().Get(Config::Texture::MenuButtonIdle),
		context.assets.Textures().Get(Config::Texture::MenuButtonSelected),
		context.localization.GetText("campaign_complete.thanks"), ButtonSize)
{
	context.window.setMouseCursorVisible(false);

	title.setOutlineThickness(3.f);
	UI::TextLayout::CenterText(title, { 960.f, 160.f });
	UI::TextLayout::FitWidth(title, TitleBounds.size.x - 240.f, 38u);

	const sf::Font& bodyFont = context.assets.Fonts().Get(context.localization.GetRegularFont(false));

	const std::array mainCopy{ "campaign_complete.line_1", "campaign_complete.line_2",
		"campaign_complete.line_3", "campaign_complete.line_4" };
	messageLines.reserve(mainCopy.size());

	for (std::size_t index = 0u; index < mainCopy.size(); index++)
	{
		messageLines.emplace_back(bodyFont, context.localization.GetText(mainCopy[index]), 36u);
		UI::TextLayout::FitWidth(messageLines.back(), MessageBounds.size.x - 220.f, 24u);
		messageLines.back().setOutlineThickness(1.5f);
		UI::TextLayout::CenterText(messageLines.back(), { 960.f, 405.f + 58.f * static_cast<float>(index) });
	}

	const std::array postscriptCopy =
	{
		"campaign_complete.postscript_1",
		"campaign_complete.postscript_2"
	};

	postscriptLines.reserve(postscriptCopy.size());

	for (std::size_t index = 0u; index < postscriptCopy.size(); index++)
	{
		postscriptLines.emplace_back(bodyFont, context.localization.GetText(postscriptCopy[index]), 31u);
		UI::TextLayout::FitWidth(postscriptLines.back(), MessageBounds.size.x - 220.f, 21u);
		postscriptLines.back().setOutlineThickness(1.f);
		UI::TextLayout::CenterText(postscriptLines.back(), { 960.f, 700.f + 48.f * static_cast<float>(index) });
	}

	button.SetPosition(ButtonPosition);
	button.SetSelected(true);
	button.SetLabelOutline(sf::Color(45, 18, 0, 220), 1.5f);

	Chrome().StartFadeIn(FadeDuration);
	ApplyReveal();
}

CampaignCompleteState::~CampaignCompleteState()
{
	GetContext().audio.StopGameplayMusic();
}

void CampaignCompleteState::HandleEvent(const sf::Event& event)
{
	if (!isInteractive || IsTransitioning() || Chrome().IsFading())
		return;

	using enum GamepadManager::NavigationAction;

	const GamepadManager::NavigationAction navigation = GetContext().gamepad.GetNavigationAction(event);

	if (navigation == Confirm || navigation == Back)
	{
		Activate();
		return;
	}

	if (const sf::Event::MouseMoved* moved = event.getIf<sf::Event::MouseMoved>())
	{
		const sf::Vector2f position = GetContext().window.mapPixelToCoords(moved->position);
		Chrome().SetMousePosition(position);
		button.SetSelected(button.Contains(position));
		return;
	}

	if (const sf::Event::KeyPressed* key = event.getIf<sf::Event::KeyPressed>())
	{
		if (key->code == sf::Keyboard::Key::Enter ||
			key->code == sf::Keyboard::Key::Space ||
			key->code == sf::Keyboard::Key::Escape)
		{
			Activate();
		}

		return;
	}

	if (const sf::Event::MouseButtonPressed* pressed = event.getIf<sf::Event::MouseButtonPressed>())
	{
		if (pressed->button == sf::Mouse::Button::Left &&
			button.Contains(GetContext().window.mapPixelToCoords(pressed->position)))
		{
			Activate();
		}
	}

}

void CampaignCompleteState::OnUpdate(float deltaTime)
{
	titleGlow.Update(deltaTime);
	buttonGlow.Update(deltaTime);

	if (!isInteractive)
	{
		revealElapsed = std::min(RevealDuration, revealElapsed + deltaTime);
		ApplyReveal();
		isInteractive = revealElapsed >= RevealDuration;
	}
}

void CampaignCompleteState::OnRender()
{
	sf::RenderWindow& window = GetContext().window;

	titleGlow.DrawBloom(
		window,
		titleFrame.GetBounds(),
		[this](sf::RenderTarget& target, const sf::RenderStates& states)
		{
			titleFrame.Draw(target, states);
			target.draw(title, states);
		},
		Gold,
		false);

	titleFrame.Draw(window);
	messageFrame.Draw(window);
	window.draw(title);

	for (const sf::Text& line : messageLines)
		window.draw(line);

	for (const sf::Text& line : postscriptLines)
		window.draw(line);

	if (isInteractive)
	{
		buttonGlow.DrawBloom(
			window,
			button.GetBounds(),
			[this](sf::RenderTarget& target, const sf::RenderStates& states)
			{
				button.Draw(target, states);
			},
			Gold);
	}

	button.Draw(window);

	if (isInteractive)
		buttonGlow.DrawHighlight(window, button.GetBounds(), Gold);
}

void CampaignCompleteState::Activate()
{
	if (IsTransitioning())
		return;

	PlayPressSound();
	BeginTransition(FadeDuration, [this]
		{
			RequestClear();
			RequestPush(StateID::MainMenu);
		});
}

void CampaignCompleteState::ApplyReveal()
{
	const float p = std::clamp(revealElapsed / RevealDuration, 0.f, 1.f);
	const float eased = p * p * (3.f - 2.f * p);
	const std::uint8_t alpha = static_cast<std::uint8_t>(255.f * eased);

	titleFrame.SetColor({ 255, 255, 255, alpha });
	messageFrame.SetColor({ 255, 255, 255, alpha });

	const auto fade = [alpha](sf::Text& text, sf::Color fill, sf::Color outline)
		{
			fill.a = alpha;
			outline.a = alpha;
			text.setFillColor(fill);
			text.setOutlineColor(outline);
		};

	fade(title, { 255, 236, 186 }, { 68, 25, 0 });

	for (sf::Text& line : messageLines)
		fade(line, { 218, 238, 244 }, { 0, 9, 17 });

	for (sf::Text& line : postscriptLines)
		fade(line, { 151, 219, 235 }, { 0, 9, 17 });

	button.SetFrameOpacity(eased);
	button.SetLabelOpacity(eased);
}