#include "AchievementToast.h"

#include <algorithm>

#include <SFML/Graphics/RenderTarget.hpp>

#include "achievements/AchievementVisuals.h"
#include "assets/Assets.h"
#include "audio/AudioManager.h"
#include "localization/LocalizationManager.h"
#include "ui/TextLayout.h"

namespace UI
{
	namespace
	{
		constexpr sf::Vector2f PanelSize = { 720.f, 154.f };
		constexpr float PanelHiddenPositionY = -180.f;
		constexpr float PanelVisiblePositionY = 32.f;
		constexpr float PanelSlideDuration = 0.42f;
		constexpr float ToastHoldDuration = 5.f;
		constexpr sf::Color GoldColor = { 255, 183, 52 };
		constexpr float PanelCornerRadius = 20.f;
		constexpr std::size_t PanelCornerPointCount = 10u;
		constexpr unsigned int UnlockedLabelCharacterSize = 20u;
		constexpr unsigned int TitleCharacterSize = 31u;
		constexpr unsigned int DescriptionCharacterSize = 21u;

		// The icon is scaled so its longer side fits this many pixels, regardless
		// of the source texture's own resolution or aspect ratio.
		constexpr float IconMaxDimension = 112.f;

		// PanelSize.x minus the description's left offset (154.f, see
		// PositionElements) minus a right-side margin, so the description never
		// overlaps the panel's rounded corner/border.
		constexpr float DescriptionMaxWidth = PanelSize.x - 174.f;
		constexpr unsigned int DescriptionMinimumCharacterSize = 15u;
	}

	AchievementToast::AchievementToast(
		Assets& store, AudioManager& audioManager, AchievementManager& manager,
		LocalizationManager& localizationManager, sf::Vector2f size)
		: assets(store)
		, audio(audioManager)
		, achievements(manager)
		, localization(localizationManager)
		, panel(PanelSize, PanelCornerRadius, PanelCornerPointCount)
		, unlockedLabel(store.Fonts().Get(localizationManager.GetBoldFont()),
			localizationManager.GetText("achievements.unlocked"), UnlockedLabelCharacterSize)
		, title(store.Fonts().Get(localizationManager.GetBoldFont()), "", TitleCharacterSize)
		, description(store.Fonts().Get(localizationManager.GetRegularFont(false)), "", DescriptionCharacterSize)
		, glowEffect(store), logicalSize(size)
	{
		panel.setFillColor({ 3, 12, 22, 245 });
		panel.setOutlineColor(GoldColor);
		panel.setOutlineThickness(3.f);

		unlockedLabel.setFillColor(GoldColor);
		title.setFillColor({ 230, 247, 251 });
		description.setFillColor({ 150, 215, 230 });

		localizationRevision.Update(localization);

		PositionElements(PanelHiddenPositionY);
	}

	void AchievementToast::Update(float dt)
	{
		glowEffect.Update(dt);

		if (localizationRevision.Update(localization))
			RefreshLocalizedContent();

		if (!isShowingToast)
		{
			if (const auto next = achievements.PopNotification())
				BeginToast(*next);

			return;
		}

		toastElapsedSeconds += dt;

		// A toast's full lifetime: slide in, hold fully visible, slide back out.
		const float totalToastDuration = PanelSlideDuration * 2.f + ToastHoldDuration;

		if (toastElapsedSeconds >= totalToastDuration)
		{
			isShowingToast = false;
			icon.reset();
			return;
		}

		// How far through the slide-in/slide-out motion the panel currently is,
		// from 0 (at PanelHiddenPositionY) to 1 (at PanelVisiblePositionY).
		// During the slide-in it counts up from 0; during the hold it stays at 1
		// (the final `else` branch doesn't apply, so it keeps the 1.f default set
		// above); during the slide-out it counts back down to 0.
		float slideProgress = 1.f;
		if (toastElapsedSeconds < PanelSlideDuration)
			slideProgress = toastElapsedSeconds / PanelSlideDuration;
		else if (toastElapsedSeconds > PanelSlideDuration + ToastHoldDuration)
			slideProgress = 1.f - (toastElapsedSeconds - PanelSlideDuration - ToastHoldDuration) / PanelSlideDuration;

		slideProgress = std::clamp(slideProgress, 0.f, 1.f);

		// "Smoothstep": remaps the linear 0..1 progress above into an S-curve, so
		// the panel eases in and out of motion instead of moving at a constant
		// speed -- looks smoother than a plain linear slide.
		const float easedSlideProgress = slideProgress * slideProgress * (3.f - 2.f * slideProgress);

		PositionElements(PanelHiddenPositionY + (PanelVisiblePositionY - PanelHiddenPositionY) * easedSlideProgress);
	}

	void AchievementToast::Draw(sf::RenderTarget& target)
	{
		if (!isShowingToast || !icon)
			return;

		glowEffect.DrawBloom(target, panel.getGlobalBounds(),
			[this](sf::RenderTarget& output, const sf::RenderStates& states)
			{
				output.draw(panel, states);
				output.draw(*icon, states);
			},
			GoldColor);

		target.draw(panel);
		target.draw(*icon);
		target.draw(unlockedLabel);
		target.draw(title);
		target.draw(description);

		glowEffect.DrawHighlight(target, panel.getGlobalBounds(), GoldColor);
	}

	void AchievementToast::BeginToast(AchievementID id)
	{
		const AchievementDefinition& definition = achievements.GetDefinition(id);

		icon.emplace(assets.Textures().Get(GetAchievementTexture(id)));

		const sf::Vector2u iconSize = icon->getTexture().getSize();
		const float iconScale = IconMaxDimension / static_cast<float>(std::max(iconSize.x, iconSize.y));
		icon->setScale({ iconScale, iconScale });

		title.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		description.setFont(assets.Fonts().Get(localization.GetRegularFont(false)));

		// Each achievement's localized text lives under "achievements.items.<id>",
		// with ".title" and ".description" suffixes for the two strings used here.
		const std::string prefix = "achievements.items." + definition.persistentID;
		title.setString(localization.GetText(prefix + ".title"));

		// The achievements page description is formatted with an explicit line
		// break for its own (narrower) tile layout; the toast panel is wider and
		// only one line tall, so the break is flattened into a space here and
		// the text is shrunk to fit instead of wrapping past the panel edge.
		sf::String description1Line = localization.GetText(prefix + ".description");

		std::size_t position = description1Line.find('\n');
		while (position != sf::String::InvalidPos)
		{
			description1Line.replace(position, 1u, " ");
			position = description1Line.find('\n', position + 1u);
		}

		description.setScale({ 1.f, 1.f });
		description.setString(description1Line);
		description.setLineSpacing(1.05f);

		TextLayout::FitWidth(description, DescriptionMaxWidth, DescriptionMinimumCharacterSize);

		toastElapsedSeconds = 0.f;
		isShowingToast = true;
		glowEffect.Invalidate();

		PositionElements(PanelHiddenPositionY);

		audio.PlaySound(Config::Sound::LevelComplete, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
	}

	void AchievementToast::RefreshLocalizedContent()
	{
		unlockedLabel.setFont(assets.Fonts().Get(localization.GetBoldFont()));
		unlockedLabel.setString(localization.GetText("achievements.unlocked"));
	}

	void AchievementToast::PositionElements(float y)
	{
		const float x = (logicalSize.x - PanelSize.x) * 0.5f;
		panel.setPosition({ x, y });

		if (icon)
			icon->setPosition({ x + 20.f, y + 21.f });

		unlockedLabel.setPosition({ x + 154.f, y + 18.f });
		title.setPosition({ x + 154.f, y + 48.f });
		description.setPosition({ x + 154.f, y + 101.f });
	}
}