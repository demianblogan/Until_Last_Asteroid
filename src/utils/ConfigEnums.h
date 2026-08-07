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
		MenuButtonIdle,
		MenuButtonSelected,

		PlayerShip,
		PlayerLife,

		BigEnemySaucer,
		SmallEnemySaucer,

		BigMeteor1,
		BigMeteor2,
		BigMeteor3,
		BigMeteor4,

		MediumMeteor1,
		MediumMeteor2,

		SmallMeteor1,
		SmallMeteor2,
		SmallMeteor3,
		SmallMeteor4,

		PlayerShot,
		EnemySaucerShot
	};

	enum class Font
	{
		GUI,
		MenuRegular,
		MenuSemibold
	};

	enum class Sound
	{
		CharacterTyping,
		InterfaceActivation,
		ItemSelect,
		ItemPress,

		PlayerLaserShot,
		EnemyLaserShot,

		SaucerKamikazeSpawn,
		SaucerShooterSpawn,

		PlayerShipExplosion,
		EnemySaucerExplosion,

		SmallMeteorExplosion,
		MediumMeteorExplosion,
		BigMeteorExplosion,

		Count
	};

	enum class Music
	{
		CompanySplash,
		MainMenuBackground,
		GameplayTheme,

		Count
	};

	enum class Shader
	{
		GaussianBlur,
		BrightPass
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
