#pragma once

// Centralized identifiers for game resources and actions.
// Used as keys in systems like AssetCache and input handling to avoid string-based lookups
// and provide type safety.
namespace Config
{
    enum class Texture
    {
        // Splash / main menu
        CompanyLogo,
        MainMenuBackground,
        MenuButtonIdle,
        MenuButtonSelected,
        MenuPointer,

        // Ship Upgrades screen
        ShipUpgradesBackground,
        ShipUpgradesHeaderDivider,
        ShipUpgradesPartsIcon,
        ShipUpgradesRowFrame,
        ShipUpgradesRowFrameSelected,
        ShipUpgradeArmorIcon,
        ShipUpgradeEnginesIcon,
        ShipUpgradeFireRateIcon,
        ShipUpgradeBonusDurationIcon,
        ShipUpgradeArmorIconSelected,
        ShipUpgradeEnginesIconSelected,
        ShipUpgradeFireRateIconSelected,
        ShipUpgradeBonusDurationIconSelected,

        // Gameplay level backgrounds (one per campaign region)
        GameplayBackgroundBlueRegion,
        GameplayBackgroundVioletRegion,
        GameplayBackgroundAsteroidRegion,
        GameplayBackgroundRedRegion,
        GameplayBackgroundDeepVoidRegion,
        GameplayBackgroundEmeraldRegion,
        GameplayBackgroundRoseRegion,
        GameplayBackgroundFrozenRegion,
        GameplayBackgroundIonRegion,
        GameplayBackgroundLastHorizon,

        // In-gameplay HUD / player
        GameplayCrosshair,
        PlayerShip,
        HealthBarFrame,
        HealthBarFill,
        ScorePanelFrame,
        GameOverTitleFrame,
        ResultTitleFrame,
        CampaignCompleteTitleFrame,
        CampaignCompletePanelFrame,

        // Pickups
        HealthPickup,
        ShieldPickup,
        HomingBulletsPickup,
        TimeSlowdownPickup,
        LaserPickup,
        TripleShotPickup,
        HelperBotPickup,
        PartToken,

        // Gamepad button prompts (Xbox / PlayStation)
        XboxLeftStick,
        XboxRightStick,
        XboxRightTrigger,
        XboxDpad,
        XboxConfirm,
        XboxBack,
        XboxMenu,
        PlayStationLeftStick,
        PlayStationRightStick,
        PlayStationRightTrigger,
        PlayStationDpad,
        PlayStationConfirm,
        PlayStationBack,
        PlayStationOptions,

        // Enemies
        BigEnemySaucer,
        SmallEnemySaucer,
        SpinnerPlatform,
        MissileCarrier,
        LaserTurret,
        ShooterStation,
        ReflectorGunship,

        // Meteors (variants for visual variety)
        BigMeteor1,
        BigMeteor2,
        BigMeteor3,
        BigMeteor4,
        SmallMeteor1,
        SmallMeteor2,
        SmallMeteor3,
        SmallMeteor4,

        // Projectiles
        PlayerShot,
        EnemySaucerShot,
        HomingMissile,

        // Boss (Level 10)
        BossCore,
        BossDiamond,
        BossOuterRing,

        // Achievement icons
        AchievementFirstStep,
        AchievementHalfwayThere,
        AchievementCampaignComplete,
        AchievementRunSurvivor,
        AchievementHordeSurvivor,
        AchievementFullyUpgraded,
        AchievementTutorialSkipped,
        AchievementFlawlessCampaign,
        AchievementBossUntouched
    };

	enum class Font
	{
		BodyRegular,
		MenuRegular,
		MenuSemibold,
		LocalizedRegular,
		LocalizedBold,
		ArabicRegular,
		ArabicBold
	};

    enum class Sound
    {
        // UI / menu feedback
        CharacterTyping,
        InterfaceActivation,
        ItemSelect,
        ItemPress,

        // Weapon fire (player and enemy)
        PlayerShot,
        PlayerLaserShot,
        EnemyShot,
        EnemyLaserShot,
        EnemyStationWorking,

        // Impacts and gameplay events
        ShipExplosion,
        AsteroidExplosion,
        BulletHitAsteroid,
        HitAsteroid,
        HitEnemySaucer,
        MetalHit,
        GameOver,
        BonusTouched,
        PartPickedUp,
        LevelComplete,

        Count
    };

	enum class Music
	{
		CompanySplash,
		MainMenuBackground,
		GameplayBackground1,
		GameplayBackground2,
		GameplayBackground3,
		BossFight,
		CampaignVictory,

		Count
	};

	enum class Shader
	{
		GaussianBlur,
		BrightPass,
		HitFlash,
		SceneBrightPass,
		SceneComposite,
		MenuVignette,
		EnemyEmission,
		PlayerEmission
	};

	enum class Cursor
	{
		MenuPointer,
		GameplayCrosshair
	};

	enum class PlayerAction
	{
		Up,
		Down,
		Left,
		Right,
		Fire
	};
};
