#include "Assets.h"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <SFML/Graphics/Image.hpp>

namespace
{
	// Pixel within each cursor image that marks the actual click point (not
	// necessarily the top-left corner) -- the menu pointer's tip sits a
	// little inside the image, while the crosshair's hotspot is its center.
	constexpr sf::Vector2u MenuPointerHotspot{ 6u, 2u };
	constexpr sf::Vector2u GameplayCrosshairHotspot{ 32u, 32u };

	sf::Cursor LoadCursor(const std::string& path, sf::Vector2u hotspot)
	{
		sf::Image image;
		if (!image.loadFromFile(path))
			throw std::runtime_error("Failed to load cursor image: " + path);

		auto cursor = sf::Cursor::createFromPixels(image.getPixelsPtr(), image.getSize(), hotspot);
		if (!cursor.has_value())
			throw std::runtime_error("Failed to create cursor: " + path);

		return std::move(cursor.value());
	}
}

bool Assets::Initialize(const ProgressCallback& progress)
{
	const auto report = [&progress](float value, std::string_view stage)
		{
			return !progress || progress(value, stage);
		};

	// Fonts load first and are cheap (header/table parsing only, no glyph
	// rasterization yet), so the loading screen can show localized text from
	// its very first frame. Gameplay data (JSON parsing) is comparatively slow,
	// especially in unoptimized Debug builds, so it gets its own checkpoint
	// instead of running silently alongside font loading before the first redraw.
	InitializeFonts();
	if (!report(0.03f, "loading.data"))
		return false;

	InitializeGameplayData();
	if (!report(0.10f, "loading.textures"))
		return false;

	InitializeTextures();
	if (!report(0.58f, "loading.sounds"))
		return false;

	InitializeSounds();
	if (!report(0.70f, "loading.music"))
		return false;

	InitializeMusic();
	if (!report(0.82f, "loading.shaders"))
		return false;

	InitializeShaders();
	if (!report(0.93f, "loading.interface"))
		return false;

	InitializeCursors();

	return report(1.f, "loading.finalizing");
}

AssetCache<sf::Texture, Config::Texture>& Assets::Textures() noexcept
{
	return textures;
}

const AssetCache<sf::Texture, Config::Texture>& Assets::Textures() const noexcept
{
	return textures;
}

AssetCache<sf::Font, Config::Font>& Assets::Fonts() noexcept
{
	return fonts;
}

const AssetCache<sf::Font, Config::Font>& Assets::Fonts() const noexcept
{
	return fonts;
}

AssetCache<sf::SoundBuffer, Config::Sound>& Assets::Sounds() noexcept
{
	return sounds;
}

const AssetCache<sf::SoundBuffer, Config::Sound>& Assets::Sounds() const noexcept
{
	return sounds;
}

AssetCache<sf::Music, Config::Music>& Assets::Music() noexcept
{
	return music;
}

const AssetCache<sf::Music, Config::Music>& Assets::Music() const noexcept
{
	return music;
}

sf::Shader& Assets::GetShader(Config::Shader id)
{
	auto found = shaders.find(id);
	if (found == shaders.end())
		throw std::runtime_error("Shader not found");

	return found->second;
}

sf::Cursor& Assets::GetCursor(Config::Cursor id)
{
	auto found = cursors.find(id);
	if (found == cursors.end())
		throw std::runtime_error("Cursor not found");

	return found->second;
}

void Assets::InitializeTextures()
{
	textures.LoadFromFile(Config::Texture::CompanyLogo, "assets/other/alone_bull_company.jpg");
	textures.LoadFromFile(Config::Texture::MainMenuBackground, "assets/backgrounds/main_menu_background.jpg");

	// Ship Upgrades art is only ever seen after the player opens that specific
	// menu, so it is loaded on first use instead of blocking the startup screen.
	textures.RegisterLazy(Config::Texture::ShipUpgradesBackground,
		"assets/backgrounds/ui/ship_upgrades_background_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradesHeaderDivider,
		"assets/sprites/ui/ship_upgrades/header_divider_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradesPartsIcon,
		"assets/sprites/ui/ship_upgrades/parts_counter_icon_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradesRowFrame,
		"assets/sprites/ui/ship_upgrades/upgrade_row_frame_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradesRowFrameSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_row_frame_selected_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeArmorIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_armor_icon_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeEnginesIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_engines_icon_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeFireRateIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_fire_rate_icon_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeBonusDurationIcon,
		"assets/sprites/ui/ship_upgrades/upgrade_bonus_duration_icon_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeArmorIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_armor_icon_selected_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeEnginesIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_engines_icon_selected_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeFireRateIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_fire_rate_icon_selected_v1_8.png", true);
	textures.RegisterLazy(Config::Texture::ShipUpgradeBonusDurationIconSelected,
		"assets/sprites/ui/ship_upgrades/upgrade_bonus_duration_icon_selected_v1_8.png", true);

	textures.RegisterLazy(Config::Texture::GameplayBackgroundBlueRegion,
		"assets/backgrounds/gameplay/blue_nebula_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundVioletRegion,
		"assets/backgrounds/gameplay/violet_clouds_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundAsteroidRegion,
		"assets/backgrounds/gameplay/asteroid_belt_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundRedRegion,
		"assets/backgrounds/gameplay/red_storm_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundDeepVoidRegion,
		"assets/backgrounds/gameplay/deep_void_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundEmeraldRegion,
		"assets/backgrounds/gameplay/emerald_aurora_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundRoseRegion,
		"assets/backgrounds/gameplay/rose_nursery_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundFrozenRegion,
		"assets/backgrounds/gameplay/frozen_expanse_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundIonRegion,
		"assets/backgrounds/gameplay/ion_storm_region.jpg", true);
	textures.RegisterLazy(Config::Texture::GameplayBackgroundLastHorizon,
		"assets/backgrounds/gameplay/last_horizon_region_v2_0.png", true);

	textures.LoadFromFile(Config::Texture::MenuButtonIdle, "assets/sprites/ui/menu_button_idle.png");
	textures.LoadFromFile(Config::Texture::MenuButtonSelected, "assets/sprites/ui/menu_button_selected.png");
	textures.LoadFromFile(Config::Texture::MenuPointer, "assets/cursors/menu_pointer.png");

	// Campaign-complete art only appears once the player finishes the campaign
	// (the panel frame is also reused by CreditsState).
	textures.RegisterLazy(Config::Texture::CampaignCompleteTitleFrame,
		"assets/sprites/ui/campaign_complete_title_panel_v2_1.png", true);
	textures.RegisterLazy(Config::Texture::CampaignCompletePanelFrame,
		"assets/sprites/ui/campaign_complete_message_panel_v2_1.png", true);

	// Achievement icons only appear on the Achievements screen. Neither is needed during startup.
	textures.RegisterLazy(Config::Texture::AchievementFirstStep,
		"assets/sprites/ui/achievements/first_step_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementHalfwayThere,
		"assets/sprites/ui/achievements/halfway_there_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementCampaignComplete,
		"assets/sprites/ui/achievements/campaign_complete_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementRunSurvivor,
		"assets/sprites/ui/achievements/run_survivor_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementHordeSurvivor,
		"assets/sprites/ui/achievements/horde_survivor_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementFullyUpgraded,
		"assets/sprites/ui/achievements/fully_upgraded_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementTutorialSkipped,
		"assets/sprites/ui/achievements/tutorial_skipped_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementFlawlessCampaign,
		"assets/sprites/ui/achievements/flawless_campaign_v2_0.png", true);
	textures.RegisterLazy(Config::Texture::AchievementBossUntouched,
		"assets/sprites/ui/achievements/boss_untouched_v2_0.png", true);

	textures.LoadFromFile(Config::Texture::GameplayCrosshair, "assets/cursors/gameplay_crosshair.png");

	textures.LoadFromFile(Config::Texture::HealthPickup, "assets/sprites/pickups/health_pickup.png");
	textures.LoadFromFile(Config::Texture::ShieldPickup, "assets/sprites/pickups/shield_pickup.png");
	textures.LoadFromFile(Config::Texture::HomingBulletsPickup, "assets/sprites/pickups/homing_bullets_pickup.png");
	textures.LoadFromFile(Config::Texture::TimeSlowdownPickup, "assets/sprites/pickups/time_slowdown_pickup.png");
	textures.LoadFromFile(Config::Texture::LaserPickup, "assets/sprites/pickups/laser_pickup_v1_8.png");
	textures.LoadFromFile(Config::Texture::TripleShotPickup, "assets/sprites/pickups/triple_shot_pickup_v1_8.png");
	textures.LoadFromFile(Config::Texture::HelperBotPickup, "assets/sprites/pickups/helper_bot_pickup_v1_9.png");
	textures.LoadFromFile(Config::Texture::PartToken, "assets/sprites/pickups/part_token_v1_8.png");

	textures.Get(Config::Texture::HealthPickup).setSmooth(true);
	textures.Get(Config::Texture::ShieldPickup).setSmooth(true);
	textures.Get(Config::Texture::HomingBulletsPickup).setSmooth(true);
	textures.Get(Config::Texture::TimeSlowdownPickup).setSmooth(true);
	textures.Get(Config::Texture::LaserPickup).setSmooth(true);
	textures.Get(Config::Texture::TripleShotPickup).setSmooth(true);
	textures.Get(Config::Texture::HelperBotPickup).setSmooth(true);
	textures.Get(Config::Texture::PartToken).setSmooth(true);

	textures.LoadFromFile(Config::Texture::PlayerShip, "assets/sprites/player/ship_v1_4.png");
	textures.Get(Config::Texture::PlayerShip).setSmooth(true);

	textures.LoadFromFile(Config::Texture::HealthBarFrame, "assets/sprites/ui/hud/health_bar_frame.png");
	textures.LoadFromFile(Config::Texture::HealthBarFill, "assets/sprites/ui/hud/health_bar_fill.png");
	textures.LoadFromFile(Config::Texture::ScorePanelFrame, "assets/sprites/ui/hud/score_panel_frame.png");
	textures.Get(Config::Texture::ScorePanelFrame).setSmooth(true);

	// Only shown after a run ends (death or level clear), never during startup.
	textures.RegisterLazy(Config::Texture::GameOverTitleFrame,
		"assets/sprites/ui/game_over/game_over_title_frame.png", true);
	textures.RegisterLazy(Config::Texture::ResultTitleFrame,
		"assets/sprites/ui/results/result_title_frame.png", true);

	const auto loadControlIcon = [this](Config::Texture id, const std::string& path)
		{
			textures.LoadFromFile(id, path);
			textures.Get(id).setSmooth(true);
		};

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

	textures.LoadFromFile(Config::Texture::BigEnemySaucer, "assets/sprites/enemies/kamikaze_saucer_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallEnemySaucer, "assets/sprites/enemies/shooter_gunship_v1_4.png");
	textures.LoadFromFile(Config::Texture::SpinnerPlatform, "assets/sprites/enemies/spinner_platform_v1_7.png");
	textures.LoadFromFile(Config::Texture::MissileCarrier, "assets/sprites/enemies/missile_carrier_v1_7.png");
	textures.LoadFromFile(Config::Texture::LaserTurret, "assets/sprites/enemies/laser_turret_v1_8.png");
	textures.LoadFromFile(Config::Texture::ShooterStation, "assets/sprites/enemies/shooter_station_v1_8.png");
	textures.LoadFromFile(Config::Texture::ReflectorGunship, "assets/sprites/enemies/reflector_gunship_v1_9.png");
	textures.LoadFromFile(Config::Texture::BossCore, "assets/sprites/enemies/boss_core_v2_0.png");
	textures.LoadFromFile(Config::Texture::BossDiamond, "assets/sprites/enemies/boss_diamond_v2_1.png");
	textures.LoadFromFile(Config::Texture::BossOuterRing, "assets/sprites/enemies/boss_outer_ring_v2_2.png");

	textures.Get(Config::Texture::BigEnemySaucer).setSmooth(true);
	textures.Get(Config::Texture::SmallEnemySaucer).setSmooth(true);
	textures.Get(Config::Texture::SpinnerPlatform).setSmooth(true);
	textures.Get(Config::Texture::MissileCarrier).setSmooth(true);
	textures.Get(Config::Texture::LaserTurret).setSmooth(true);
	textures.Get(Config::Texture::ShooterStation).setSmooth(true);
	textures.Get(Config::Texture::ReflectorGunship).setSmooth(true);
	textures.Get(Config::Texture::BossCore).setSmooth(true);
	textures.Get(Config::Texture::BossDiamond).setSmooth(true);
	textures.Get(Config::Texture::BossOuterRing).setSmooth(true);

	textures.LoadFromFile(Config::Texture::BigMeteor1, "assets/sprites/meteors/large_asteroid_01_v1_4.png");
	textures.LoadFromFile(Config::Texture::BigMeteor2, "assets/sprites/meteors/large_asteroid_02_v1_4.png");
	textures.LoadFromFile(Config::Texture::BigMeteor3, "assets/sprites/meteors/large_asteroid_03_v1_4.png");
	textures.LoadFromFile(Config::Texture::BigMeteor4, "assets/sprites/meteors/large_asteroid_04_v1_4.png");

	textures.Get(Config::Texture::BigMeteor1).setSmooth(true);
	textures.Get(Config::Texture::BigMeteor2).setSmooth(true);
	textures.Get(Config::Texture::BigMeteor3).setSmooth(true);
	textures.Get(Config::Texture::BigMeteor4).setSmooth(true);

	textures.LoadFromFile(Config::Texture::SmallMeteor1, "assets/sprites/meteors/small_asteroid_01_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor2, "assets/sprites/meteors/small_asteroid_02_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor3, "assets/sprites/meteors/small_asteroid_03_v1_4.png");
	textures.LoadFromFile(Config::Texture::SmallMeteor4, "assets/sprites/meteors/small_asteroid_04_v1_4.png");

	textures.Get(Config::Texture::SmallMeteor1).setSmooth(true);
	textures.Get(Config::Texture::SmallMeteor2).setSmooth(true);
	textures.Get(Config::Texture::SmallMeteor3).setSmooth(true);
	textures.Get(Config::Texture::SmallMeteor4).setSmooth(true);

	textures.LoadFromFile(Config::Texture::PlayerShot, "assets/sprites/shots/player_projectile_v1_4.png");
	textures.LoadFromFile(Config::Texture::EnemySaucerShot, "assets/sprites/shots/enemy_projectile_v1_4.png");
	textures.LoadFromFile(Config::Texture::HomingMissile, "assets/sprites/shots/homing_missile_v1_7.png");

	textures.Get(Config::Texture::PlayerShot).setSmooth(true);
	textures.Get(Config::Texture::EnemySaucerShot).setSmooth(true);
	textures.Get(Config::Texture::HomingMissile).setSmooth(true);
}

const GameplayData& Assets::GetGameplayData() const
{
	if (!gameplayData.has_value())
		throw std::runtime_error("Gameplay data is not initialized");

	return gameplayData.value();
}

void Assets::InitializeGameplayData()
{
	gameplayData.emplace("assets/data/gameplay");
}

void Assets::InitializeFonts()
{
	fonts.LoadFromFile(Config::Font::BodyRegular, "assets/fonts/Exo2-Regular.ttf");
	fonts.LoadFromFile(Config::Font::MenuRegular, "assets/fonts/orbitron_regular.ttf");
	fonts.LoadFromFile(Config::Font::MenuSemibold, "assets/fonts/orbitron_semibold.ttf");
	fonts.LoadFromFile(Config::Font::LocalizedRegular, "assets/fonts/NotoSans-Regular.ttf");
	fonts.LoadFromFile(Config::Font::LocalizedBold, "assets/fonts/NotoSans-Bold.ttf");
	fonts.LoadFromFile(Config::Font::ArabicRegular, "assets/fonts/NotoSansArabic-Regular.ttf");
	fonts.LoadFromFile(Config::Font::ArabicBold, "assets/fonts/NotoSansArabic-Bold.ttf");
}

void Assets::InitializeSounds()
{
	sounds.LoadFromFile(Config::Sound::CharacterTyping, "assets/audio/sounds/character_typing.ogg");
	sounds.LoadFromFile(Config::Sound::InterfaceActivation, "assets/audio/sounds/interface_activation.ogg");
	sounds.LoadFromFile(Config::Sound::ItemSelect, "assets/audio/sounds/item_select.ogg");
	sounds.LoadFromFile(Config::Sound::ItemPress, "assets/audio/sounds/item_press.ogg");

	sounds.LoadFromFile(Config::Sound::PlayerShot, "assets/audio/sounds/player_normal_shot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyShot, "assets/audio/sounds/enemy_shot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyLaserShot, "assets/audio/sounds/enemy_laser_shot.ogg");
	sounds.LoadFromFile(Config::Sound::EnemyStationWorking, "assets/audio/sounds/enemy_station_working.ogg");
	sounds.LoadFromFile(Config::Sound::PlayerLaserShot, "assets/audio/sounds/player_laser_shot.ogg");

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

void Assets::InitializeMusic()
{
	music.LoadFromFile(Config::Music::CompanySplash, "assets/audio/music/company_splash.ogg");
	music.LoadFromFile(Config::Music::MainMenuBackground, "assets/audio/music/main_menu_background.ogg");
	music.LoadFromFile(Config::Music::GameplayBackground1, "assets/audio/music/gameplay_background_1.ogg");
	music.LoadFromFile(Config::Music::GameplayBackground2, "assets/audio/music/gameplay_background_2.ogg");
	music.LoadFromFile(Config::Music::GameplayBackground3, "assets/audio/music/gameplay_background_3.ogg");
	music.LoadFromFile(Config::Music::BossFight, "assets/audio/sounds/boss_fight.ogg");
	music.LoadFromFile(Config::Music::CampaignVictory, "assets/audio/sounds/campaign_victory.ogg");
}

void Assets::InitializeShaders()
{
	sf::Shader blurShader;
	const std::string blurPath = "assets/shaders/gaussian_blur.frag";
	if (!blurShader.loadFromFile(blurPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + blurPath);

	shaders.emplace(Config::Shader::GaussianBlur, std::move(blurShader));

	sf::Shader brightPassShader;
	const std::string brightPassPath = "assets/shaders/bright_pass.frag";
	if (!brightPassShader.loadFromFile(brightPassPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + brightPassPath);

	shaders.emplace(Config::Shader::BrightPass, std::move(brightPassShader));

	sf::Shader hitFlashShader;
	const std::string hitFlashPath = "assets/shaders/hit_flash.frag";
	if (!hitFlashShader.loadFromFile(hitFlashPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + hitFlashPath);

	shaders.emplace(Config::Shader::HitFlash, std::move(hitFlashShader));

	sf::Shader sceneBrightPassShader;
	const std::string sceneBrightPassPath = "assets/shaders/scene_bright_pass.frag";
	if (!sceneBrightPassShader.loadFromFile(sceneBrightPassPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + sceneBrightPassPath);

	shaders.emplace(Config::Shader::SceneBrightPass, std::move(sceneBrightPassShader));

	sf::Shader sceneCompositeShader;
	const std::string sceneCompositePath = "assets/shaders/gameplay_post_process.frag";
	if (!sceneCompositeShader.loadFromFile(sceneCompositePath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + sceneCompositePath);

	shaders.emplace(Config::Shader::SceneComposite, std::move(sceneCompositeShader));

	sf::Shader menuVignetteShader;
	const std::string menuVignettePath = "assets/shaders/menu_vignette.frag";
	if (!menuVignetteShader.loadFromFile(menuVignettePath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + menuVignettePath);

	shaders.emplace(Config::Shader::MenuVignette, std::move(menuVignetteShader));

	sf::Shader enemyEmissionShader;
	const std::string enemyEmissionPath = "assets/shaders/enemy_emission.frag";
	if (!enemyEmissionShader.loadFromFile(enemyEmissionPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + enemyEmissionPath);

	shaders.emplace(Config::Shader::EnemyEmission, std::move(enemyEmissionShader));

	sf::Shader playerEmissionShader;
	const std::string playerEmissionPath = "assets/shaders/player_emission.frag";
	if (!playerEmissionShader.loadFromFile(playerEmissionPath, sf::Shader::Type::Fragment))
		throw std::runtime_error("Failed to load shader: " + playerEmissionPath);

	shaders.emplace(Config::Shader::PlayerEmission, std::move(playerEmissionShader));
}

void Assets::InitializeCursors()
{
	cursors.emplace(
		Config::Cursor::MenuPointer,
		LoadCursor("assets/cursors/menu_pointer.png", MenuPointerHotspot)
	);

	cursors.emplace(
		Config::Cursor::GameplayCrosshair,
		LoadCursor("assets/cursors/gameplay_crosshair.png", GameplayCrosshairHotspot)
	);
}