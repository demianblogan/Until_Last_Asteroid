#include "GameplayEffects.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include "assets/AssetStore.h"
#include "core/World.h"
#include "utils/ConfigEnums.h"

namespace
{
    sf::Vector2f Normalize(const sf::Vector2f& vector)
    {
        const float lengthSquared{ vector.x * vector.x + vector.y * vector.y };
        if (lengthSquared <= 0.0001f)
            return { 0.f, -1.f };
        return vector / std::sqrt(lengthSquared);
    }

    int ScaledCount(int count, float scale)
    {
        return std::max(1, static_cast<int>(std::round(static_cast<float>(count) * scale)));
    }

    constexpr float ScorePopupDuration{ 0.5f };
    constexpr float ScorePopupRiseDistance{ 56.f };
}

GameplayEffects::ScorePopup::ScorePopup(
    const sf::Font& font, int points, sf::Vector2f position)
    : text(font, "+" + std::to_string(points), 30u)
{
    text.setFillColor(sf::Color(225, 252, 255));
    text.setOutlineColor(sf::Color(20, 175, 255, 230));
    text.setOutlineThickness(2.f);
    const sf::FloatRect bounds{ text.getLocalBounds() };
    text.setOrigin({
        bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f });
    text.setPosition(position);
}

GameplayEffects::GameplayEffects(const GameplayData::EffectsConfig& config, AssetStore& assets)
    : config(config)
    , scorePopupFont(assets.Fonts().Get(Config::Font::MenuSemibold))
{
}

void GameplayEffects::Update(
    float deltaTime, World& world, bool screenShakeEnabled, bool showScorePopups)
{
    if (!showScorePopups)
        scorePopups.clear();

    shakeEnabled = screenShakeEnabled;
    if (!shakeEnabled)
    {
        shakeRemaining = 0.f;
        shakeDuration = 0.f;
        shakeAmplitude = 0.f;
        cameraOffset = {};
    }
    projectileGlowParticles.Clear();

    constexpr float EmissionInterval{ 1.f / 70.f };
    const auto playerState{ world.GetPlayerEffectState() };
    if (!playerState || !playerState->isThrusting)
    {
        engineEmissionAccumulator = 0.f;
    }
    else
    {
        engineEmissionAccumulator += deltaTime;
        int emittedSteps{ 0 };
        while (engineEmissionAccumulator >= EmissionInterval && emittedSteps < 8)
        {
            EmitPlayerEngineParticles(world);
            engineEmissionAccumulator -= EmissionInterval;
            ++emittedSteps;
        }
        engineEmissionAccumulator = std::min(engineEmissionAccumulator, EmissionInterval);
    }

    for (const World::EffectEvent& event : world.GetEffectEvents())
    {
        switch (event.type)
        {
        case World::EffectEventType::PlayerProjectileGlow:
            EmitProjectileGlow(true, event.position, event.direction);
            break;
		case World::EffectEventType::PlayerHomingProjectileGlow:
			EmitProjectileGlow(true, event.position, event.direction, true);
			break;
        case World::EffectEventType::EnemyProjectileGlow:
            EmitProjectileGlow(false, event.position, event.direction);
            break;
		case World::EffectEventType::MissileSmoke:
			EmitMissileSmoke(event.position, event.direction);
			break;
		case World::EffectEventType::EnemyEngine:
			EmitEnemyEngine(event.position, event.direction);
			break;
        case World::EffectEventType::PlayerMuzzleFlash:
            EmitMuzzleFlash(true, event.position, event.direction);
            break;
        case World::EffectEventType::EnemyMuzzleFlash:
            EmitMuzzleFlash(false, event.position, event.direction);
            break;
        case World::EffectEventType::AsteroidHit:
            EmitStoneHit(event.position, event.direction, event.scale);
            break;
        case World::EffectEventType::ShipHit:
            EmitMetalHit(event.position, event.direction, event.scale);
            break;
        case World::EffectEventType::PlayerHit:
            EmitMetalHit(event.position, event.direction, event.scale);
            postProcessState.damageVignette = 1.f;
            StartCameraShake(config.damageShake, event.scale);
            break;
        case World::EffectEventType::AsteroidExplosion:
            EmitAsteroidExplosion(event.position, event.scale);
            break;
        case World::EffectEventType::ShipExplosion:
            EmitShipExplosion(event.position, event.scale);
            break;
        case World::EffectEventType::ScorePopup:
            if (showScorePopups)
                EmitScorePopup(event.position, event.value);
            break;
        }
    }
    world.ClearEffectEvents();

    engineParticles.Update(deltaTime);
    projectileGlowParticles.Update(0.f);
    weaponParticles.Update(deltaTime);
    impactParticles.Update(deltaTime);
    smokeParticles.Update(deltaTime);
    debrisParticles.Update(deltaTime);
    shockwaveParticles.Update(deltaTime);
    UpdateScorePopups(deltaTime);
    UpdateCameraShake(deltaTime);
    UpdatePostProcess(deltaTime);
}

void GameplayEffects::DrawBehindEntities(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(smokeParticles, states);
    target.draw(engineParticles, states);
    target.draw(projectileGlowParticles, states);
    target.draw(shockwaveParticles, states);
}

void GameplayEffects::DrawAboveEntities(sf::RenderTarget& target, sf::RenderStates states) const
{
    target.draw(weaponParticles, states);
    target.draw(debrisParticles, states);
    target.draw(impactParticles, states);
    for (const ScorePopup& popup : scorePopups)
        target.draw(popup.text, states);
}

void GameplayEffects::Clear()
{
    engineEmissionAccumulator = 0.f;
    shakeRemaining = 0.f;
    shakeDuration = 0.f;
    shakeAmplitude = 0.f;
    cameraOffset = {};
    postProcessState = {};
	shockwaves.clear();
    engineParticles.Clear();
    projectileGlowParticles.Clear();
    weaponParticles.Clear();
    impactParticles.Clear();
    smokeParticles.Clear();
    debrisParticles.Clear();
    shockwaveParticles.Clear();
    scorePopups.clear();
}

std::size_t GameplayEffects::GetParticleCount() const noexcept
{
    return engineParticles.GetParticleCount() + projectileGlowParticles.GetParticleCount() +
        weaponParticles.GetParticleCount() + impactParticles.GetParticleCount() +
        smokeParticles.GetParticleCount() + debrisParticles.GetParticleCount() +
        shockwaveParticles.GetParticleCount();
}

sf::Vector2f GameplayEffects::GetCameraOffset() const noexcept
{
    return cameraOffset;
}

const GameplayEffects::PostProcessState& GameplayEffects::GetPostProcessState() const noexcept
{
    return postProcessState;
}

float GameplayEffects::RandomFloat(float minimum, float maximum)
{
    return std::uniform_real_distribution<float>(minimum, maximum)(random);
}

sf::Vector2f GameplayEffects::RandomDirection()
{
    const float angle{ RandomFloat(0.f, 2.f * std::numbers::pi_v<float>) };
    return { std::cos(angle), std::sin(angle) };
}

sf::Vector2f GameplayEffects::RandomDirectionAround(
    const sf::Vector2f& direction, float spreadRadians)
{
    const sf::Vector2f normalized{ Normalize(direction) };
    const float angle{ std::atan2(normalized.y, normalized.x) +
        RandomFloat(-spreadRadians, spreadRadians) };
    return { std::cos(angle), std::sin(angle) };
}

void GameplayEffects::EmitPlayerEngineParticles(const World& world)
{
    const auto playerState{ world.GetPlayerEffectState() };
    if (!playerState)
        return;

    const sf::Vector2f perpendicular{
        -playerState->exhaustDirection.y,
        playerState->exhaustDirection.x };

    for (const sf::Vector2f& emitterPosition : playerState->enginePositions)
    {
        const float sidewaysJitter{ RandomFloat(-24.f, 24.f) };
        const float exhaustSpeed{ RandomFloat(150.f, 235.f) };
        const float lifetime{ RandomFloat(0.2f, 0.32f) };
        engineParticles.Emit({
            emitterPosition + perpendicular * RandomFloat(-1.5f, 1.5f),
            playerState->velocity * 0.18f +
                playerState->exhaustDirection * exhaustSpeed +
                perpendicular * sidewaysJitter,
            lifetime,
            RandomFloat(14.f, 19.f),
            RandomFloat(2.f, 4.f),
            { 155, 255, 255, 250 },
            { 25, 95, 255, 0 },
            0.f,
            0.f,
            2.5f });
    }
}

void GameplayEffects::EmitProjectileGlow(bool playerProjectile,
    const sf::Vector2f& position, const sf::Vector2f& direction, bool homing)
{
    const sf::Color startColor{ homing
		? sf::Color{ 255, 188, 45, 240 }
		: playerProjectile
            ? sf::Color{ 85, 235, 255, 235 }
            : sf::Color{ 255, 55, 110, 235 } };

    (void)direction;
    projectileGlowParticles.Emit({
        position, { 0.f, 0.f }, 1.f,
        playerProjectile ? 34.f : 31.f,
        playerProjectile ? 34.f : 31.f,
        startColor,
        startColor });
}

void GameplayEffects::EmitMissileSmoke(
	const sf::Vector2f& position, const sf::Vector2f& direction)
{
	const sf::Vector2f normalized{ Normalize(direction) };
	const sf::Vector2f perpendicular{ -normalized.y, normalized.x };
	const sf::Vector2f exhaustPosition{ position - normalized * 20.f };
	smokeParticles.Emit({
		exhaustPosition + perpendicular * RandomFloat(-2.5f, 2.5f),
		-normalized * RandomFloat(35.f, 65.f) +
			perpendicular * RandomFloat(-22.f, 22.f),
		RandomFloat(0.38f, 0.62f),
		RandomFloat(13.f, 19.f),
		RandomFloat(25.f, 34.f),
		{ 125, 130, 145, 155 },
		{ 35, 38, 48, 0 },
		0.f,
		RandomFloat(-0.8f, 0.8f),
		1.2f });
	weaponParticles.Emit({
		exhaustPosition,
		-normalized * RandomFloat(30.f, 55.f),
		RandomFloat(0.08f, 0.13f),
		RandomFloat(13.f, 17.f),
		RandomFloat(4.f, 7.f),
		{ 255, 245, 135, 255 },
		{ 255, 90, 15, 0 },
		0.f,
		0.f,
		2.f });
}

void GameplayEffects::EmitEnemyEngine(
	const sf::Vector2f& position, const sf::Vector2f& direction)
{
	const sf::Vector2f normalized{ Normalize(direction) };
	const sf::Vector2f perpendicular{ -normalized.y, normalized.x };
	engineParticles.Emit({
		position + perpendicular * RandomFloat(-1.5f, 1.5f),
		normalized * RandomFloat(125.f, 190.f) +
			perpendicular * RandomFloat(-20.f, 20.f),
		RandomFloat(0.18f, 0.28f),
		RandomFloat(11.f, 16.f),
		RandomFloat(2.f, 4.f),
		{ 255, 105, 75, 245 },
		{ 130, 12, 28, 0 },
		0.f,
		0.f,
		2.2f });
}

void GameplayEffects::EmitMuzzleFlash(bool playerProjectile,
    const sf::Vector2f& position, const sf::Vector2f& direction)
{
    const sf::Color coreColor{ playerProjectile
        ? sf::Color{ 185, 255, 255, 255 }
        : sf::Color{ 255, 175, 195, 255 } };
    const sf::Color fadeColor{ playerProjectile
        ? sf::Color{ 30, 135, 255, 0 }
        : sf::Color{ 255, 25, 75, 0 } };

    weaponParticles.Emit({
        position + direction * 3.f,
        direction * RandomFloat(15.f, 35.f),
        RandomFloat(0.14f, 0.18f),
        playerProjectile ? 58.f : 50.f,
        5.f,
        coreColor,
        fadeColor,
        0.f,
        0.f,
        5.f });

    const sf::Vector2f perpendicular{ -direction.y, direction.x };
    weaponParticles.Emit({
        position + direction * 7.f,
        direction * RandomFloat(35.f, 65.f),
        RandomFloat(0.1f, 0.14f),
        playerProjectile ? 27.f : 24.f,
        2.f,
        sf::Color::White,
        fadeColor });

    for (int i{ 0 }; i < 8; ++i)
    {
        weaponParticles.Emit({
            position,
            direction * RandomFloat(80.f, 175.f) +
                perpendicular * RandomFloat(-95.f, 95.f),
            RandomFloat(0.08f, 0.16f),
            RandomFloat(5.f, 9.f),
            RandomFloat(1.f, 2.5f),
            coreColor,
            fadeColor,
            0.f,
            0.f,
            3.f });
    }
}

void GameplayEffects::EmitStoneHit(
    const sf::Vector2f& position, const sf::Vector2f& direction, float scale)
{
    const auto& burst{ config.stoneHit };
    impactParticles.Emit({ position, {}, 0.16f, 34.f * scale, 5.f,
        { 255, 210, 125, 220 }, { 150, 80, 25, 0 } });

    for (int i{ 0 }; i < ScaledCount(burst.count, scale); ++i)
    {
        const sf::Vector2f particleDirection{ RandomDirectionAround(direction, 1.35f) };
        const float size{ RandomFloat(burst.minimumSize, burst.maximumSize) * scale };
        debrisParticles.Emit({
            position,
            particleDirection * RandomFloat(burst.minimumSpeed, burst.maximumSpeed),
            RandomFloat(burst.minimumLifetime, burst.maximumLifetime),
            size,
            size * 0.45f,
            { 185, 150, 105, 245 },
            { 75, 55, 40, 0 },
            RandomFloat(0.f, 2.f * std::numbers::pi_v<float>),
            RandomFloat(-8.f, 8.f),
            2.2f });
    }

    for (int i{ 0 }; i < std::max(1, ScaledCount(burst.count, scale) / 5); ++i)
    {
        smokeParticles.Emit({
            position + RandomDirection() * RandomFloat(0.f, 8.f),
            RandomDirectionAround(direction, 1.5f) * RandomFloat(20.f, 65.f),
            RandomFloat(0.3f, 0.55f),
            RandomFloat(18.f, 28.f) * scale,
            RandomFloat(32.f, 48.f) * scale,
            { 135, 115, 90, 150 },
            { 45, 40, 38, 0 },
            0.f, 0.f, 1.4f });
    }
}

void GameplayEffects::EmitMetalHit(
    const sf::Vector2f& position, const sf::Vector2f& direction, float scale)
{
    const auto& burst{ config.metalHit };
    impactParticles.Emit({ position, {}, 0.13f, 42.f * scale, 3.f,
        { 220, 250, 255, 255 }, { 30, 145, 255, 0 } });

    for (int i{ 0 }; i < ScaledCount(burst.count, scale); ++i)
    {
        const sf::Vector2f sparkDirection{ RandomDirectionAround(direction, 1.55f) };
        const float speed{ RandomFloat(burst.minimumSpeed, burst.maximumSpeed) };
        const float angle{ std::atan2(sparkDirection.y, sparkDirection.x) };
        impactParticles.Emit({
            position,
            sparkDirection * speed,
            RandomFloat(burst.minimumLifetime, burst.maximumLifetime),
            RandomFloat(burst.minimumSize, burst.maximumSize) * scale,
            0.8f,
            { 210, 250, 255, 255 },
            { 20, 110, 255, 0 },
            angle,
            0.f,
            0.8f,
            RandomFloat(2.5f, 4.5f) });
    }
}

void GameplayEffects::EmitAsteroidExplosion(const sf::Vector2f& position, float scale)
{
    const bool large{ scale >= 0.8f };
    const auto& burst{ large ? config.largeAsteroidExplosion : config.smallAsteroidExplosion };
    impactParticles.Emit({ position, {}, 0.34f, 145.f * scale, 8.f,
        { 255, 235, 175, 255 }, { 255, 80, 15, 0 } });
    impactParticles.Emit({ position, {}, 0.5f, 85.f * scale, 185.f * scale,
        { 255, 120, 30, 190 }, { 120, 25, 5, 0 } });

    for (int i{ 0 }; i < ScaledCount(burst.count, scale); ++i)
    {
        const sf::Vector2f direction{ RandomDirection() };
        const float size{ RandomFloat(burst.minimumSize, burst.maximumSize) };
        debrisParticles.Emit({
            position + direction * RandomFloat(0.f, 10.f * scale),
            direction * RandomFloat(burst.minimumSpeed, burst.maximumSpeed),
            RandomFloat(burst.minimumLifetime, burst.maximumLifetime),
            size,
            size * 0.55f,
            { 195, 155, 105, 255 },
            { 55, 42, 35, 0 },
            RandomFloat(0.f, 2.f * std::numbers::pi_v<float>),
            RandomFloat(-7.f, 7.f),
            1.6f });
    }

    const int smokeCount{ std::max(3, ScaledCount(burst.count, scale) / 5) };
    for (int i{ 0 }; i < smokeCount; ++i)
    {
        const sf::Vector2f direction{ RandomDirection() };
        smokeParticles.Emit({
            position + direction * RandomFloat(0.f, 22.f * scale),
            direction * RandomFloat(25.f, 95.f),
            RandomFloat(0.65f, 1.25f),
            RandomFloat(28.f, 50.f) * scale,
            RandomFloat(65.f, 105.f) * scale,
            { 100, 82, 70, 185 },
            { 28, 27, 30, 0 },
            RandomFloat(0.f, 2.f * std::numbers::pi_v<float>),
            RandomFloat(-1.3f, 1.3f),
            0.9f });
    }

    if (large)
    {
        shockwaveParticles.Emit({ position, {}, 0.42f, 42.f, 245.f * scale,
            { 255, 190, 95, 205 }, { 255, 80, 20, 0 } });
        StartCameraShake(config.largeExplosionShake, 0.8f * scale);
        StartShockwave(position, scale);
    }
}

void GameplayEffects::EmitShipExplosion(const sf::Vector2f& position, float scale)
{
    const auto& burst{ config.shipExplosion };
    impactParticles.Emit({ position, {}, 0.28f, 165.f * scale, 9.f,
        { 235, 255, 255, 255 }, { 25, 130, 255, 0 } });
    impactParticles.Emit({ position, {}, 0.52f, 75.f * scale, 205.f * scale,
        { 80, 210, 255, 210 }, { 255, 40, 20, 0 } });

    for (int i{ 0 }; i < ScaledCount(burst.count, scale); ++i)
    {
        const sf::Vector2f direction{ RandomDirection() };
        const float speed{ RandomFloat(burst.minimumSpeed, burst.maximumSpeed) };
        if (i % 2 == 0)
        {
            const float angle{ std::atan2(direction.y, direction.x) };
            impactParticles.Emit({
                position,
                direction * speed,
                RandomFloat(burst.minimumLifetime, burst.maximumLifetime) * 0.65f,
                RandomFloat(2.f, 5.f),
                0.7f,
                { 220, 250, 255, 255 },
                { 255, 65, 20, 0 },
                angle, 0.f, 0.8f, RandomFloat(2.5f, 5.f) });
        }
        else
        {
            const float size{ RandomFloat(burst.minimumSize, burst.maximumSize) };
            debrisParticles.Emit({
                position,
                direction * speed * 0.72f,
                RandomFloat(burst.minimumLifetime, burst.maximumLifetime),
                size,
                size * 0.5f,
                { 135, 175, 190, 255 },
                { 45, 52, 60, 0 },
                RandomFloat(0.f, 2.f * std::numbers::pi_v<float>),
                RandomFloat(-11.f, 11.f),
                1.2f });
        }
    }

    const int smokeCount{ std::max(4, ScaledCount(burst.count, scale) / 6) };
    for (int i{ 0 }; i < smokeCount; ++i)
    {
        const sf::Vector2f direction{ RandomDirection() };
        smokeParticles.Emit({
            position + direction * RandomFloat(0.f, 20.f * scale),
            direction * RandomFloat(25.f, 105.f),
            RandomFloat(0.7f, 1.35f),
            RandomFloat(26.f, 48.f) * scale,
            RandomFloat(70.f, 115.f) * scale,
            { 80, 88, 98, 190 },
            { 20, 23, 30, 0 },
            RandomFloat(0.f, 2.f * std::numbers::pi_v<float>),
            RandomFloat(-1.5f, 1.5f),
            0.8f });
    }

    shockwaveParticles.Emit({ position, {}, 0.46f, 48.f, 265.f * scale,
        { 120, 225, 255, 220 }, { 255, 65, 30, 0 } });
    StartCameraShake(config.largeExplosionShake, scale);
    StartShockwave(position, scale);
}

void GameplayEffects::EmitScorePopup(const sf::Vector2f& position, int points)
{
    scorePopups.emplace_back(scorePopupFont, points, position);
}

void GameplayEffects::UpdateScorePopups(float deltaTime)
{
    for (ScorePopup& popup : scorePopups)
    {
        popup.elapsed += deltaTime;
        const float progress{ std::clamp(popup.elapsed / ScorePopupDuration, 0.f, 1.f) };
        popup.text.move({ 0.f, -ScorePopupRiseDistance * deltaTime / ScorePopupDuration });
        const float fade{ 1.f - progress * progress };
        const auto alpha{ static_cast<std::uint8_t>(255.f * fade) };
        popup.text.setFillColor(sf::Color(225, 252, 255, alpha));
        popup.text.setOutlineColor(sf::Color(
            20, 175, 255, static_cast<std::uint8_t>(230.f * fade)));
    }

    std::erase_if(scorePopups, [](const ScorePopup& popup)
    {
        return popup.elapsed >= ScorePopupDuration;
    });
}

void GameplayEffects::StartCameraShake(
    const GameplayData::CameraShakeConfig& shake, float scale)
{
    if (!shakeEnabled)
        return;
    shakeDuration = std::max(shakeDuration, shake.duration);
    shakeRemaining = std::max(shakeRemaining, shake.duration);
    shakeAmplitude = std::min(12.f, std::max(shakeAmplitude, shake.amplitude * scale));
}

void GameplayEffects::UpdateCameraShake(float deltaTime)
{
    if (shakeRemaining <= 0.f || shakeDuration <= 0.f)
    {
        cameraOffset = {};
        shakeAmplitude = 0.f;
        return;
    }

    shakeRemaining = std::max(0.f, shakeRemaining - deltaTime);
    const float falloff{ shakeRemaining / shakeDuration };
    const sf::Vector2f target{
        RandomFloat(-shakeAmplitude, shakeAmplitude) * falloff,
        RandomFloat(-shakeAmplitude, shakeAmplitude) * falloff };
    cameraOffset = cameraOffset * 0.3f + target * 0.7f;
}

void GameplayEffects::StartShockwave(const sf::Vector2f& position, float scale)
{
	if (shockwaves.size() >= MaximumShockwaves)
		shockwaves.erase(shockwaves.begin());
	shockwaves.push_back({
		position,
		0.f,
		0.55f,
		std::clamp(scale, 0.7f, 1.35f) });
}

void GameplayEffects::UpdatePostProcess(float deltaTime)
{
    postProcessState.damageVignette = std::max(
        0.f,
        postProcessState.damageVignette - deltaTime * 2.6f);

	for (Shockwave& shockwave : shockwaves)
		shockwave.elapsed += deltaTime;
	std::erase_if(shockwaves, [](const Shockwave& shockwave)
	{
		return shockwave.elapsed >= shockwave.duration;
	});

	postProcessState.shockwaveCount = std::min(
		shockwaves.size(), MaximumShockwaves);
	for (std::size_t index{ 0u }; index < postProcessState.shockwaveCount; ++index)
	{
		const Shockwave& shockwave{ shockwaves[index] };
		const float progress{ std::clamp(
			shockwave.elapsed / shockwave.duration, 0.f, 1.f) };
		// The explosion particles are rendered with the current camera-shake
		// transform, so the post-process center must use the same offset.
		postProcessState.shockwavePositions[index] = shockwave.position + cameraOffset;
		postProcessState.shockwaveRadii[index] = std::lerp(
			24.f, 285.f * shockwave.scale, progress);
		postProcessState.shockwaveStrengths[index] = (1.f - progress) * 0.9f;
	}
}
