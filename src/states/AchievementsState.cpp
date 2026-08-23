#include "AchievementsState.h"
#include <algorithm>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include "achievements/AchievementManager.h"
#include "achievements/AchievementVisuals.h"
#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "input/GamepadManager.h"
#include "ui/TextLayout.h"

namespace
{
	constexpr sf::Vector2f TileSize{ 540.f, 218.f };
	constexpr sf::Vector2f FirstTile{ 105.f, 145.f };
	constexpr sf::Vector2f TileSpacing{ 585.f, 240.f };
	constexpr sf::Vector2f ButtonSize{ 540.f, 92.f };
	constexpr sf::Color Cyan{ 25, 220, 255 };
	constexpr sf::Color Gold{ 255, 178, 42 };

	void Center(sf::Text& text, sf::Vector2f position)
	{
		const auto bounds{ text.getLocalBounds() };
		text.setOrigin(bounds.position + bounds.size * 0.5f);
		text.setPosition(position);
	}

}

AchievementsState::AchievementsState(StateStack& stack, StateContext context)
	: State(stack, context), background(context.assets, context.logicalSize)
	, titleGlow(context.assets)
	, buttonGlow(context.assets)
	, cursor(context.assets, Config::Texture::MenuPointer, { 6.f, 2.f }, Cyan)
	, fade(context.logicalSize)
	, title(context.assets.Fonts().Get(context.localization.BoldFont()),
		context.localization.Get("achievements.title"), 68u)
	, returnButton(context.assets.Fonts().Get(context.localization.RegularFont()),
		context.assets.Textures().Get(Config::Texture::MenuButtonIdle),
		context.assets.Textures().Get(Config::Texture::MenuButtonSelected),
		context.localization.Get("common.back_main"), ButtonSize)
{
	context.window.setMouseCursorVisible(false);
	title.setFillColor({ 215, 247, 252 });
	title.setOutlineColor({ 3, 18, 31, 235 });
	title.setOutlineThickness(3.5f);

	const auto& definitions{ context.achievements.GetDefinitions() };
	const sf::Font& titleFont{ context.assets.Fonts().Get(context.localization.BoldFont()) };
	const sf::Font& bodyFont{ context.assets.Fonts().Get(context.localization.RegularFont(false)) };
	tiles.reserve(definitions.size()); icons.reserve(definitions.size());
	achievementTitles.reserve(definitions.size()); descriptions.reserve(definitions.size());
	for (std::size_t index{ 0u }; index < definitions.size(); ++index)
	{
		const auto& definition{ definitions[index] };
		const sf::Vector2f position{ FirstTile + sf::Vector2f{
			TileSpacing.x * static_cast<float>(index % 3u),
			TileSpacing.y * static_cast<float>(index / 3u) } };
		tiles.emplace_back(TileSize, 18.f, 10u);
		tiles.back().setPosition(position);
		tiles.back().setOutlineThickness(2.5f);
		icons.emplace_back(context.assets.Textures().Get(GetAchievementTexture(definition.id)));
		const auto textureSize{ icons.back().getTexture().getSize() };
		const float scale{ 154.f / static_cast<float>(std::max(textureSize.x, textureSize.y)) };
		icons.back().setScale({ scale, scale });
		icons.back().setPosition(position + sf::Vector2f{ 22.f, 32.f });
		achievementTitles.emplace_back(titleFont, "", 27u);
		achievementTitles.back().setPosition(position + sf::Vector2f{ 198.f, 43.f });
		descriptions.emplace_back(bodyFont, "", 21u);
		descriptions.back().setPosition(position + sf::Vector2f{ 198.f, 94.f });
		descriptions.back().setLineSpacing(1.18f);
	}
	RefreshLocalizedContent();
	RefreshUnlockState();

	returnButton.SetPosition({ 690.f, 895.f });
	returnButton.SetSelected(false);
	fade.StartFadeIn(0.35f);
}

void AchievementsState::HandleEvent(const sf::Event& event)
{
	if (returning || fade.IsActive()) return;
	using enum GamepadManager::NavigationAction;
	const auto navigation{ GetContext().gamepad.GetNavigationAction(event) };
	if (navigation == Confirm || navigation == Back) { BeginReturn(); return; }
	if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
	{
		const auto point{ GetContext().window.mapPixelToCoords(moved->position) };
		background.SetMousePosition(point);
		const bool wasSelected{ returnButtonSelected };
		returnButtonSelected = returnButton.Contains(point);
		returnButton.SetSelected(returnButtonSelected);
		if (returnButtonSelected != wasSelected)
			buttonGlow.Invalidate();
	}
	if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
		if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space ||
			key->code == sf::Keyboard::Key::Escape) BeginReturn();
	if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() })
		if (pressed->button == sf::Mouse::Button::Left && returnButton.Contains(
			GetContext().window.mapPixelToCoords(pressed->position))) BeginReturn();
}

void AchievementsState::Update(float dt)
{
	background.Update(dt); titleGlow.Update(dt); buttonGlow.Update(dt); cursor.Update(dt); fade.Update(dt);
	if (GetContext().gamepad.IsInUse() && !returnButtonSelected)
	{
		returnButtonSelected = true;
		returnButton.SetSelected(true);
		buttonGlow.Invalidate();
	}
	if (returning && !fade.IsActive()) RequestPop();
}

void AchievementsState::Render()
{
	auto& window{ GetContext().window };
	background.Draw(window);
	titleGlow.DrawBloom(window, title.getGlobalBounds(),
		[this](sf::RenderTarget& target, const sf::RenderStates& states)
		{ target.draw(title, states); }, Cyan);
	window.draw(title);
	for (std::size_t i{ 0u }; i < tiles.size(); ++i)
	{
		window.draw(tiles[i]); window.draw(icons[i]);
		window.draw(achievementTitles[i]); window.draw(descriptions[i]);
	}
	if (returnButtonSelected)
		buttonGlow.DrawBloom(window, returnButton.GetBounds(),
			[this](sf::RenderTarget& target, const sf::RenderStates& states)
			{ returnButton.Draw(target, states); }, Gold);
	returnButton.Draw(window);
	if (returnButtonSelected)
		buttonGlow.DrawHighlight(window, returnButton.GetBounds(), Gold);
}

void AchievementsState::RenderOverlay()
{
	if (!GetContext().gamepad.IsInUse()) cursor.Draw(GetContext().window);
	fade.Draw(GetContext().window);
}

void AchievementsState::OnReactivated()
{
	GetContext().window.setMouseCursorVisible(false);
	if (localizationRevision != GetContext().localization.GetRevision())
		RefreshLocalizedContent();
	returning = false;
	returnButtonSelected = false;
	returnButton.SetSelected(false);
	buttonGlow.Invalidate();
	RefreshUnlockState();
	fade.StartFadeIn(0.35f);
}

void AchievementsState::RefreshLocalizedContent()
{
	const StateContext& context{ GetContext() };
	localizationRevision = context.localization.GetRevision();

	title.setFont(context.assets.Fonts().Get(context.localization.BoldFont()));
	title.setString(context.localization.Get("achievements.title"));
	Center(title, { 960.f, 78.f });
	titleGlow.Invalidate();

	const auto& definitions{ context.achievements.GetDefinitions() };
	const sf::Font& titleFont{ context.assets.Fonts().Get(context.localization.BoldFont()) };
	const sf::Font& bodyFont{ context.assets.Fonts().Get(context.localization.RegularFont(false)) };
	for (std::size_t index{ 0u }; index < definitions.size() && index < achievementTitles.size(); ++index)
	{
		const auto& definition{ definitions[index] };
		const std::string prefix{ "achievements.items." + definition.persistentID };

		achievementTitles[index].setFont(titleFont);
		achievementTitles[index].setString(context.localization.Get(prefix + ".title"));
		achievementTitles[index].setScale({ 1.f, 1.f });
		TextLayout::FitWidth(achievementTitles[index], 315.f, 18u);

		descriptions[index].setFont(bodyFont);
		descriptions[index].setString(context.localization.Get(prefix + ".description"));
		descriptions[index].setScale({ 1.f, 1.f });
		TextLayout::FitWidth(descriptions[index], 315.f, 16u);
	}

	returnButton.SetFont(context.assets.Fonts().Get(context.localization.RegularFont()));
	returnButton.SetLabel(context.localization.Get("common.back_main"));
}

void AchievementsState::RefreshUnlockState()
{
	const auto& definitions{ GetContext().achievements.GetDefinitions() };
	for (std::size_t index{ 0u }; index < definitions.size() && index < tiles.size(); ++index)
	{
		const bool unlocked{ GetContext().achievements.IsUnlocked(definitions[index].id) };
		tiles[index].setFillColor(unlocked ? sf::Color(3, 15, 26, 238) : sf::Color(32, 36, 40, 238));
		tiles[index].setOutlineColor(unlocked ? Gold : sf::Color(85, 90, 95));
		icons[index].setColor(unlocked ? sf::Color::White : sf::Color(72, 72, 72));
		achievementTitles[index].setFillColor(unlocked ? Gold : sf::Color(125, 125, 125));
		descriptions[index].setFillColor(unlocked ? sf::Color(195, 225, 232) : sf::Color(105, 105, 105));
	}
}

void AchievementsState::BeginReturn()
{
	if (returning) return;
	GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI,
		100.f, 1.f, SoundPlayback::StopPrevious);
	returning = true;
	fade.StartFadeOut(0.35f);
}
