#include "AssetStore.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <SFML/Graphics/Image.hpp>

namespace
{
	sf::Cursor LoadCursor(const std::string& path, sf::Vector2u hotspot)
	{
		sf::Image image;
		if (!image.loadFromFile(path))
			throw std::runtime_error("Failed to load cursor image: " + path);

		auto cursor{ sf::Cursor::createFromPixels(image.getPixelsPtr(), image.getSize(), hotspot) };
		if (!cursor.has_value())
			throw std::runtime_error("Failed to create cursor: " + path);

		return std::move(cursor.value());
	}
}

void AssetStore::Initialize()
{
	InitializeGameplayData();
	InitializeTextures();
	InitializeFonts();
	InitializeSounds();
	InitializeMusic();
	InitializeShaders();
	InitializeCursors();
}

AssetStorage<sf::Texture, Config::Texture>& AssetStore::Textures() noexcept
{
	return textures;
}

const AssetStorage<sf::Texture, Config::Texture>& AssetStore::Textures() const noexcept
{
	return textures;
}

AssetStorage<sf::Font, Config::Font>& AssetStore::Fonts() noexcept
{
	return fonts;
}

const AssetStorage<sf::Font, Config::Font>& AssetStore::Fonts() const noexcept
{
	return fonts;
}

AssetStorage<sf::SoundBuffer, Config::Sound>& AssetStore::Sounds() noexcept
{
	return sounds;
}

const AssetStorage<sf::SoundBuffer, Config::Sound>& AssetStore::Sounds() const noexcept
{
	return sounds;
}

AssetStorage<sf::Music, Config::Music>& AssetStore::Music() noexcept
{
	return music;
}

const AssetStorage<sf::Music, Config::Music>& AssetStore::Music() const noexcept
{
	return music;
}

sf::Shader& AssetStore::GetShader(Config::Shader id)
{
	auto iterator{ shaders.find(id) };
	if (iterator == shaders.end())
		throw std::runtime_error("Shader not found");

	return iterator->second;
}

sf::Cursor& AssetStore::GetCursor(Config::Cursor id)
{
	auto iterator{ cursors.find(id) };
	if (iterator == cursors.end())
		throw std::runtime_error("Cursor not found");

	return iterator->second;
}

void AssetStore::InitializeTextures()
{
	textures.LoadFromFile(Config::Texture::CompanyLogo, "assets/other/alone_bull_company.png");
	textures.LoadFromFile(Config::Texture::MainMenuBackground, "assets/backgrounds/main_menu_background.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradesBackground,
		"assets/backgrounds/ui/ship_upgrades_background_v1_8.png");
	textures.Get(Config::Texture::ShipUpgradesBackground).setSmooth(true);
	textures.LoadFromFile(Config::Texture::ShipUpgradesHeaderDivider,
		"assets/sprites/ui/ship_upgrades/header_divider_v1_8.png");
	textures.Get(Config::Texture::ShipUpgradesHeaderDivider).setSmooth(true);
	textures.LoadFromFile(Config::Texture::ShipUpgradesPartsIcon,
		"assets/sprites/ui/ship_upgrades/parts_counter_icon_v1_8.png");
	textures.Get(Config::Texture::ShipUpgradesPartsIcon).setSmooth(true);
	textures.LoadFromFile(Config::Texture::ShipUpgradesRowFrame,
		"assets/sprites/ui/ship_upgrades/upgrade_row_frame_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradesRowFrameSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_row_frame_selected_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeArmorIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_armor_icon_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeEnginesIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_engines_icon_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeFireRateIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_fire_rate_icon_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeBonusDurationIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_bonus_duration_icon_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeArmorIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_armor_icon_selected_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeEnginesIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_engines_icon_selected_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeFireRateIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_fire_rate_icon_selected_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShipUpgradeBonusDurationIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_bonus_duration_icon_selected_v1_8.png");
	textures.Get(Config::Texture::ShipUpgradesRowFrame).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradesRowFrameSelected).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeArmorIcon).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeEnginesIcon).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeFireRateIcon).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeBonusDurationIcon).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeArmorIconSelected).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeEnginesIconSelected).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeFireRateIconSelected).setSmooth(true);
	textures.Get(Config::Texture::ShipUpgradeBonusDurationIconSelected).setSmooth(true);
	textures.LoadFromFile(Config::Texture::GameplayBackgroundBlueRegion,
		"assets/backgrounds/gameplay/blue_nebula_region.jpg");
	textures.LoadFromFile(Config::Texture::GameplayBackgroundVioletRegion,
		"assets/backgrounds/gameplay/violet_clouds_region.jpg");
	textures.LoadFromFile(Config::Texture::GameplayBackgroundAsteroidRegion,
		"assets/backgrounds/gameplay/asteroid_belt_region.jpg");
	textures.LoadFromFile(Config::Texture::GameplayBackgroundRedRegion,
		"assets/backgrounds/gameplay/red_storm_region.jpg");
	textures.LoadFromFile(Config::Texture::GameplayBackgroundDeepVoidRegion,
		"assets/backgrounds/gameplay/deep_void_region.jpg");
	textures.Get(Config::Texture::GameplayBackgroundBlueRegion).setSmooth(true);
	textures.Get(Config::Texture::GameplayBackgroundVioletRegion).setSmooth(true);
	textures.Get(Config::Texture::GameplayBackgroundAsteroidRegion).setSmooth(true);
	textures.Get(Config::Texture::GameplayBackgroundRedRegion).setSmooth(true);
	textures.Get(Config::Texture::GameplayBackgroundDeepVoidRegion).setSmooth(true);
	textures.LoadFromFile(Config::Texture::MenuButtonIdle, "assets/sprites/ui/menu_button_idle.png");
	textures.LoadFromFile(Config::Texture::MenuButtonSelected, "assets/sprites/ui/menu_button_selected.png");
	textures.LoadFromFile(Config::Texture::MenuPointer, "assets/cursors/menu_pointer.png");
	textures.LoadFromFile(Config::Texture::GameplayCrosshair, "assets/cursors/gameplay_crosshair.png");
	textures.LoadFromFile(Config::Texture::HealthPickup, "assets/sprites/pickups/health_pickup.png");
	textures.LoadFromFile(Config::Texture::ShieldPickup, "assets/sprites/pickups/shield_pickup.png");
	textures.LoadFromFile(Config::Texture::HomingBulletsPickup,
		"assets/sprites/pickups/homing_bullets_pickup.png");
	textures.LoadFromFile(Config::Texture::TimeSlowdownPickup,
		"assets/sprites/pickups/time_slowdown_pickup.png");
	textures.LoadFromFile(Config::Texture::LaserPickup,
		"assets/sprites/pickups/laser_pickup_v1_8.png");
	textures.LoadFromFile(Config::Texture::TripleShotPickup,
		"assets/sprites/pickups/triple_shot_pickup_v1_8.png");
	textures.LoadFromFile(Config::Texture::PartToken,
		"assets/sprites/pickups/part_token_v1_8.png");
	textures.Get(Config::Texture::HealthPickup).setSmooth(true);
	textures.Get(Config::Texture::ShieldPickup).setSmooth(true);
	textures.Get(Config::Texture::HomingBulletsPickup).setSmooth(true);
	textures.Get(Config::Texture::TimeSlowdownPickup).setSmooth(true);
	textures.Get(Config::Texture::LaserPickup).setSmooth(true);
	textures.Get(Config::Texture::TripleShotPickup).setSmooth(true);
	textures.Get(Config::Texture::PartToken).setSmooth(true);

	textures.LoadFromFile(Config::Texture::PlayerShip, "assets/sprites/player/ship_v1_4.png");
	textures.Get(Config::Texture::PlayerShip).setSmooth(true);
	textures.LoadFromFile(Config::Texture::PlayerLife, "assets/sprites/player/life.png");
	textures.LoadFromFile(Config::Texture::HealthBarFrame, "assets/sprites/ui/hud/health_bar_frame.png");
	textures.LoadFromFile(Config::Texture::HealthBarFill, "assets/sprites/ui/hud/health_bar_fill.png");
	textures.LoadFromFile(Config::Texture::ScorePanelFrame, "assets/sprites/ui/hud/score_panel_frame.png");
	textures.Get(Config::Texture::ScorePanelFrame).setSmooth(true);
	textures.LoadFromFile(Config::Texture::GameOverTitleFrame,
		"assets/sprites/ui/game_over/game_over_title_frame.png");
	textures.Get(Config::Texture::GameOverTitleFrame).setSmooth(true);
	textures.LoadFromFile(Config::Texture::ResultTitleFrame,
		"assets/sprites/ui/results/result_title_frame.png");
	textures.Get(Config::Texture::ResultTitleFrame).setSmooth(true);
	const auto loadControlIcon{ [this](Config::Texture id, const std::string& path)
	{
		textures.LoadFromFile(id, path);
		textures.Get(id).setSmooth(true);
	} };
	loadControlIcon(Config::Texture::XboxLeftStick, "assets/sprites/ui/controls/xbox_ls.png");
	loadControlIcon(Config::Texture::XboxRightStick, "assets/sprites/ui/controls/xbox_rs.png");
	loadControlIcon(Config::Texture::XboxRightTrigger, "assets/sprites/ui/controls/xbox_rt.png");
	loadControlIcon(Config::Texture::XboxDpad, "assets/sprites/ui/controls/xbox_dpad.png");
	loadControlIcon(Config::Texture::XboxConfirm, "assets/sprites/ui/controls/xbox_a.png");
	loadControlIcon(Config::Texture::XboxBack, "assets/sprites/ui/controls/xbox_b.png");
	loadControlIcon(Config::Texture::XboxMenu, "assets/sprites/ui/controls/xbox_menu.png");
	loadControlIcon(Config::Texture::PlayStationLeftStick, "assets/sprites/ui/controls/playstation_l.png");
	loadControlIcon(Config::Texture::PlayStationRightStick, "assets/sprites/ui/controls/playstation_r.png");
	loadControlIcon(Config::Texture::PlayStationRightTrigger, "assets/sprites/ui/controls/playstation_r2.png");
	loadControlIcon(Config::Texture::PlayStationDpad, "assets/sprites/ui/controls/playstation_dpad.png");
	loadControlIcon(Config::Texture::PlayStationConfirm, "assets/sprites/ui/controls/playstation_cross.png");
	loadControlIcon(Config::Texture::PlayStationBack, "assets/sprites/ui/controls/playstation_circle.png");
	loadControlIcon(Config::Texture::PlayStationOptions, "assets/sprites/ui/controls/playstation_options.png");

	textures.LoadFromFile(Config::Texture::BigEnemySaucer,
		"assets/sprites/enemies/kamikaze_saucer_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallEnemySaucer,
		"assets/sprites/enemies/shooter_gunship_v1_4.png");
	textures.LoadFromFile(Config::Texture::SpinnerPlatform,
		"assets/sprites/enemies/spinner_platform_v1_7.png");
	textures.LoadFromFile(Config::Texture::MissileCarrier,
		"assets/sprites/enemies/missile_carrier_v1_7.png");
	textures.LoadFromFile(Config::Texture::LaserTurret,
		"assets/sprites/enemies/laser_turret_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShooterStation,
		"assets/sprites/enemies/shooter_station_v1_8.png");
	textures.Get(Config::Texture::BigEnemySaucer).setSmooth(true);
	textures.Get(Config::Texture::SmallEnemySaucer).setSmooth(true);
	textures.Get(Config::Texture::SpinnerPlatform).setSmooth(true);
	textures.Get(Config::Texture::MissileCarrier).setSmooth(true);
	textures.Get(Config::Texture::LaserTurret).setSmooth(true);
	textures.Get(Config::Texture::ShooterStation).setSmooth(true);

	textures.LoadFromFile(Config::Texture::BigMeteor1,
		"assets/sprites/meteors/large_asteroid_01_v1_4.png");
	textures.LoadFromFile(Config::Texture::BigMeteor2,
		"assets/sprites/meteors/large_asteroid_02_v1_4.png");
	textures.LoadFromFile(Config::Texture::BigMeteor3,
		"assets/sprites/meteors/large_asteroid_03_v1_4.png");
	textures.LoadFromFile(Config::Texture::BigMeteor4,
		"assets/sprites/meteors/large_asteroid_04_v1_4.png");
	textures.Get(Config::Texture::BigMeteor1).setSmooth(true);
	textures.Get(Config::Texture::BigMeteor2).setSmooth(true);
	textures.Get(Config::Texture::BigMeteor3).setSmooth(true);
	textures.Get(Config::Texture::BigMeteor4).setSmooth(true);

	textures.LoadFromFile(Config::Texture::SmallMeteor1,
		"assets/sprites/meteors/small_asteroid_01_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor2,
		"assets/sprites/meteors/small_asteroid_02_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor3,
		"assets/sprites/meteors/small_asteroid_03_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor4,
		"assets/sprites/meteors/small_asteroid_04_v1_4.png");
	textures.Get(Config::Texture::SmallMeteor1).setSmooth(true);
	textures.Get(Config::Texture::SmallMeteor2).setSmooth(true);
	textures.Get(Config::Texture::SmallMeteor3).setSmooth(true);
	textures.Get(Config::Texture::SmallMeteor4).setSmooth(true);

	textures.LoadFromFile(Config::Texture::PlayerShot,
		"assets/sprites/shots/player_projectile_v1_4.png");
	textures.LoadFromFile(Config::Texture::EnemySaucerShot,
		"assets/sprites/shots/enemy_projectile_v1_4.png");
	textures.LoadFromFile(Config::Texture::HomingMissile,
		"assets/sprites/shots/homing_missile_v1_7.png");
	textures.Get(Config::Texture::PlayerShot).setSmooth(true);
	textures.Get(Config::Texture::EnemySaucerShot).setSmooth(true);
	textures.Get(Config::Texture::HomingMissile).setSmooth(true);
}

const GameplayData& AssetStore::GetGameplayData() const
{
	if (!gameplayData.has_value())
		throw std::runtime_error("Gameplay data is not initialized");

	return gameplayData.value();
}

void AssetStore::InitializeGameplayData()
{
	gameplayData.emplace("assets/data/gameplay");
}

void AssetStore::InitializeFonts()
{
	fonts.LoadFromFile(Config::Font::GUI, "assets/fonts/trs_million.ttf");
	fonts.LoadFromFile(Config::Font::BodyRegular, "assets/fonts/Exo2-Regular.ttf");
	fonts.LoadFromFile(Config::Font::MenuRegular, "assets/fonts/orbitron_regular.ttf");
	fonts.LoadFromFile(Config::Font::MenuSemibold, "assets/fonts/orbitron_semibold.ttf");
}

void AssetStore::InitializeSounds()
{
	sounds.LoadFromFile(Config::Sound::CharacterTyping, "assets/audio/sounds/character_typing.ogg");
	sounds.LoadFromFile(Config::Sound::InterfaceActivation, "assets/audio/sounds/interface_activation.ogg");
	sounds.LoadFromFile(Config::Sound::ItemSelect, "assets/audio/sounds/item_select.ogg");
	sounds.LoadFromFile(Config::Sound::ItemPress, "assets/audio/sounds/item_press.ogg");

	sounds.LoadFromFile(Config::Sound::PlayerShot, "assets/audio/sounds/player_normal_shot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyShot, "assets/audio/sounds/enemy_shot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyLaserShot, "assets/audio/sounds/enemy_laser_shot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyStationWorking, "assets/audio/sounds/enemy_station_working.ogg");

	sounds.LoadFromFile(Config::Sound::ShipExplosion, "assets/audio/sounds/enemy_saucer_explosion.ogg");
	sounds.LoadFromFile(Config::Sound::AsteroidExplosion, "assets/audio/sounds/asteroid_explosion.ogg");
	sounds.LoadFromFile(Config::Sound::BulletHitAsteroid, "assets/audio/sounds/bullet_hit_asteroid.ogg");
	sounds.LoadFromFile(Config::Sound::HitAsteroid, "assets/audio/sounds/hit_asteroid.ogg");
	sounds.LoadFromFile(Config::Sound::HitEnemySaucer, "assets/audio/sounds/hit_enemy_saucer.ogg");
	sounds.LoadFromFile(Config::Sound::MetalHit, "assets/audio/sounds/metal_hit.ogg");
	sounds.LoadFromFile(Config::Sound::GameOver, "assets/audio/sounds/game_over.ogg");
	sounds.LoadFromFile(Config::Sound::BonusTouched, "assets/audio/sounds/bonus_touched.ogg");
	sounds.LoadFromFile(Config::Sound::PartPickedUp, "assets/audio/sounds/parts_picked_up.ogg");
	sounds.LoadFromFile(Config::Sound::LevelComplete, "assets/audio/sounds/level_complete.ogg");
}

void AssetStore::InitializeMusic()
{
	music.LoadFromFile(Config::Music::CompanySplash, "assets/audio/music/company_splash.ogg");
	music.LoadFromFile(Config::Music::MainMenuBackground, "assets/audio/music/main_menu_background.ogg");
	music.LoadFromFile(Config::Music::GameplayBackground1, "assets/audio/music/gameplay_background_1.ogg");
}

void AssetStore::InitializeShaders()
{
	sf::Shader blurShader;
	const std::string blurPath{ "assets/shaders/gaussian_blur.frag" };
	if (!blurShader.loadFromFile(blurPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + blurPath);

	shaders.emplace(Config::Shader::GaussianBlur, std::move(blurShader));

	sf::Shader brightPassShader;
	const std::string brightPassPath{ "assets/shaders/bright_pass.frag" };
	if (!brightPassShader.loadFromFile(brightPassPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + brightPassPath);

	shaders.emplace(Config::Shader::BrightPass, std::move(brightPassShader));

	sf::Shader hitFlashShader;
	const std::string hitFlashPath{ "assets/shaders/hit_flash.frag" };
	if (!hitFlashShader.loadFromFile(hitFlashPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + hitFlashPath);

	shaders.emplace(Config::Shader::HitFlash, std::move(hitFlashShader));

	sf::Shader sceneBrightPassShader;
	const std::string sceneBrightPassPath{ "assets/shaders/scene_bright_pass.frag" };
	if (!sceneBrightPassShader.loadFromFile(sceneBrightPassPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + sceneBrightPassPath);

	shaders.emplace(Config::Shader::SceneBrightPass, std::move(sceneBrightPassShader));

	sf::Shader sceneCompositeShader;
	const std::string sceneCompositePath{ "assets/shaders/gameplay_post_process.frag" };
	if (!sceneCompositeShader.loadFromFile(sceneCompositePath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + sceneCompositePath);

	shaders.emplace(Config::Shader::SceneComposite, std::move(sceneCompositeShader));

	sf::Shader menuVignetteShader;
	const std::string menuVignettePath{ "assets/shaders/menu_vignette.frag" };
	if (!menuVignetteShader.loadFromFile(menuVignettePath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + menuVignettePath);

	shaders.emplace(Config::Shader::MenuVignette, std::move(menuVignetteShader));

	sf::Shader enemyEmissionShader;
	const std::string enemyEmissionPath{ "assets/shaders/enemy_emission.frag" };
	if (!enemyEmissionShader.loadFromFile(enemyEmissionPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + enemyEmissionPath);

	shaders.emplace(Config::Shader::EnemyEmission, std::move(enemyEmissionShader));

	sf::Shader playerEmissionShader;
	const std::string playerEmissionPath{ "assets/shaders/player_emission.frag" };
	if (!playerEmissionShader.loadFromFile(playerEmissionPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + playerEmissionPath);

	shaders.emplace(Config::Shader::PlayerEmission, std::move(playerEmissionShader));
}

void AssetStore::InitializeCursors()
{
	cursors.emplace(
		Config::Cursor::MenuPointer,
		LoadCursor("assets/cursors/menu_pointer.png", { 6u, 2u }));

	cursors.emplace(
		Config::Cursor::GameplayCrosshair,
		LoadCursor("assets/cursors/gameplay_crosshair.png", { 32u, 32u }));
}
