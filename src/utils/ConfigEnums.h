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
		GameplayBackgroundLevel1,
		GameplayBackgroundLevel2,
		GameplayBackgroundLevel3,
		GameplayBackgroundLevel4,
		GameplayBackgroundLevel5,
		MenuButtonIdle,
		MenuButtonSelected,
		MenuPointer,
		GameplayCrosshair,
		HealthPickup,
		ShieldPickup,
		HomingBulletsPickup,
		TimeSlowdownPickup,

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
		HomingMissile
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

		ShipExplosion,
		AsteroidExplosion,
		BulletHitAsteroid,
		HitAsteroid,
		HitEnemySaucer,
		MetalHit,
		GameOver,
		BonusTouched,
		Countdown,
		LevelComplete,

		Count
	};

	enum class Music
	{
		CompanySplash,
		MainMenuBackground,
		GameplayBackground1,

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
