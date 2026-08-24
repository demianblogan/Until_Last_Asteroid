#include "AchievementToast.h"
#include <algorithm>
#include <SFML/Graphics/RenderTarget.hpp>
#include "achievements/AchievementVisuals.h"
#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "ui/TextLayout.h"

namespace
{
	constexpr sf::Vector2f PanelSize{ 720.f, 154.f };
	constexpr float HiddenY{ -180.f };
	constexpr float VisibleY{ 32.f };
	constexpr float MoveDuration{ 0.42f };
	constexpr float HoldDuration{ 5.f };
	constexpr sf::Color Gold{ 255, 183, 52 };
}

AchievementToast::AchievementToast(Assets& store, AudioManager& audioManager,
	AchievementManager& manager, LocalizationManager& localizationManager, sf::Vector2f size)
	: assets(store), audio(audioManager), achievements(manager), localization(localizationManager)
	, panel(PanelSize, 20.f, 10u)
	, unlockedLabel(store.Fonts().Get(localizationManager.GetBoldFont()),
		localizationManager.GetText("achievements.unlocked"), 20u)
	, title(store.Fonts().Get(localizationManager.GetBoldFont()), "", 31u)
	, description(store.Fonts().Get(localizationManager.GetRegularFont(false)), "", 21u)
	, glow(store), logicalSize(size)
{
	panel.setFillColor({ 3, 12, 22, 245 });
	panel.setOutlineColor(Gold);
	panel.setOutlineThickness(3.f);
	unlockedLabel.setFillColor(Gold);
	title.setFillColor({ 230, 247, 251 });
	description.setFillColor({ 150, 215, 230 });
	localizationRevision = localization.GetLanguageRevision();
	Layout(HiddenY);
}

void AchievementToast::Update(float dt)
{
	glow.Update(dt);
	if (localizationRevision != localization.GetLanguageRevision())
		RefreshLocalizedContent();
	if (!active)
	{
		if (const auto next{ achievements.PopNotification() }) Begin(*next);
		return;
	}
	elapsed += dt;
	const float total{ MoveDuration * 2.f + HoldDuration };
	if (elapsed >= total) { active = false; icon.reset(); return; }
	float progress{ 1.f };
	if (elapsed < MoveDuration) progress = elapsed / MoveDuration;
	else if (elapsed > MoveDuration + HoldDuration)
		progress = 1.f - (elapsed - MoveDuration - HoldDuration) / MoveDuration;
	progress = std::clamp(progress, 0.f, 1.f);
	const float eased{ progress * progress * (3.f - 2.f * progress) };
	Layout(HiddenY + (VisibleY - HiddenY) * eased);
}

void AchievementToast::Draw(sf::RenderTarget& target)
{
	if (!active || !icon) return;
	glow.DrawBloom(target, panel.getGlobalBounds(),
		[this](sf::RenderTarget& output, const sf::RenderStates& states)
		{ output.draw(panel, states); output.draw(*icon, states); }, Gold);
	target.draw(panel); target.draw(*icon); target.draw(unlockedLabel);
	target.draw(title); target.draw(description);
	glow.DrawHighlight(target, panel.getGlobalBounds(), Gold);
}

void AchievementToast::Begin(AchievementID id)
{
	const auto& definition{ achievements.GetDefinition(id) };
	icon.emplace(assets.Textures().Get(GetAchievementTexture(id)));
	const auto size{ icon->getTexture().getSize() };
	const float scale{ 112.f / static_cast<float>(std::max(size.x, size.y)) };
	icon->setScale({ scale, scale });
	title.setFont(assets.Fonts().Get(localization.GetBoldFont()));
	description.setFont(assets.Fonts().Get(localization.GetRegularFont(false)));
	const std::string prefix{ "achievements.items." + definition.persistentID };
	title.setString(localization.GetText(prefix + ".title"));

	// The achievements page description is formatted with an explicit line
	// break for its own (narrower) tile layout; the toast panel is wider and
	// only one line tall, so the break is flattened into a space here and
	// the text is shrunk to fit instead of wrapping past the panel edge.
	sf::String description1Line{ localization.GetText(prefix + ".description") };
	for (std::size_t position{ description1Line.find('\n') };
		position != sf::String::InvalidPos;
		position = description1Line.find('\n', position + 1u))
		description1Line.replace(position, 1u, " ");
	description.setScale({ 1.f, 1.f });
	description.setString(description1Line);
	description.setLineSpacing(1.05f);
	TextLayout::FitWidth(description, PanelSize.x - 174.f, 15u);
	elapsed = 0.f; active = true; glow.Invalidate(); Layout(HiddenY);
	audio.PlaySound(Config::Sound::LevelComplete, SoundGroup::UI,
		100.f, 1.f, SoundPlayback::StopPrevious);
}

void AchievementToast::RefreshLocalizedContent()
{
	localizationRevision = localization.GetLanguageRevision();
	unlockedLabel.setFont(assets.Fonts().Get(localization.GetBoldFont()));
	unlockedLabel.setString(localization.GetText("achievements.unlocked"));
}

void AchievementToast::Layout(float y)
{
	const float x{ (logicalSize.x - PanelSize.x) * 0.5f };
	panel.setPosition({ x, y });
	if (icon) icon->setPosition({ x + 20.f, y + 21.f });
	unlockedLabel.setPosition({ x + 154.f, y + 18.f });
	title.setPosition({ x + 154.f, y + 48.f });
	description.setPosition({ x + 154.f, y + 101.f });
}
