#pragma once

// Centralized identifiers for game resources and actions.
// Used as keys in systems like AssetStorage and input handling to avoid string-based lookups
// and provide type safety.
namespace Config
{
	enum class Texture
	{
		CompanyLogo,
		MainMenuBackground,
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
		GameplayBackgroundBlueRegion,
		GameplayBackgroundVioletRegion,
		GameplayBackgroundAsteroidRegion,
		GameplayBackgroundRedRegion,
		GameplayBackgroundDeepVoidRegion,
		MenuButtonIdle,
		MenuButtonSelected,
		MenuPointer,
		GameplayCrosshair,
		HealthPickup,
		ShieldPickup,
		HomingBulletsPickup,
		TimeSlowdownPickup,
		LaserPickup,
		TripleShotPickup,
		HelperBotPickup,
		PartToken,

		PlayerShip,
		PlayerLife,
		HealthBarFrame,
		HealthBarFill,
		ScorePanelFrame,
		GameOverTitleFrame,
		ResultTitleFrame,
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

		BigEnemySaucer,
		SmallEnemySaucer,
		SpinnerPlatform,
		MissileCarrier,
		LaserTurret,
		ShooterStation,
		ReflectorGunship,

		BigMeteor1,
		BigMeteor2,
		BigMeteor3,
		BigMeteor4,

		SmallMeteor1,
		SmallMeteor2,
		SmallMeteor3,
		SmallMeteor4,

		PlayerShot,
		EnemySaucerShot,
		HomingMissile,

		// Append new texture identifiers here. Existing enum values are used as
		// resource keys and must remain stable across incremental builds.
		GameplayBackgroundEmeraldRegion,
		GameplayBackgroundRoseRegion,
		GameplayBackgroundFrozenRegion,
		GameplayBackgroundIonRegion
	};

	enum class Font
	{
		GUI,
		BodyRegular,
		MenuRegular,
		MenuSemibold
	};

	enum class Sound
	{
		CharacterTyping,
		InterfaceActivation,
		ItemSelect,
		ItemPress,

		PlayerShot,
		EnemyShot,
		EnemyLaserShot,
		EnemyStationWorking,

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
		PlayerLaserShot,

		Count
	};

	enum class Music
	{
		CompanySplash,
		MainMenuBackground,
		GameplayBackground1,
		GameplayBackground2,
		GameplayBackground3,

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
