#pragma once

#include "achievements/AchievementManager.h"
#include "utils/ConfigEnums.h"

// Kept separate from AchievementManager.h on purpose: AchievementManager is
// pure data/logic (progress, unlocking, save file) and doesn't know
// anything about rendering. This file is the UI-side mapping from an
// achievement to its icon texture, so only the UI code that actually draws
// achievements (AchievementToast, AchievementsState) needs to depend on it.

inline Config::Texture GetAchievementTexture(AchievementID id)
{
	switch (id)
	{
	case AchievementID::FirstStep:
		return Config::Texture::AchievementFirstStep;
	case AchievementID::HalfwayThere: 
		return Config::Texture::AchievementHalfwayThere;
	case AchievementID::CampaignComplete: 
		return Config::Texture::AchievementCampaignComplete;
	case AchievementID::RunSurvivor: 
		return Config::Texture::AchievementRunSurvivor;
	case AchievementID::HordeSurvivor: 
		return Config::Texture::AchievementHordeSurvivor;
	case AchievementID::FullyUpgraded:
		return Config::Texture::AchievementFullyUpgraded;
	case AchievementID::TutorialSkipped: 
		return Config::Texture::AchievementTutorialSkipped;
	case AchievementID::FlawlessCampaign: 
		return Config::Texture::AchievementFlawlessCampaign;
	case AchievementID::BossUntouched: 
		return Config::Texture::AchievementBossUntouched;

	default: 
		return Config::Texture::AchievementFirstStep;
	}
}