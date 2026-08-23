#include "Player.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include "assets/Assets.h"
#include "core/World.h"
#include "gameplay/GameplaySession.h"
#include "input/GamepadManager.h"

namespace
{
	constexpr float PlayerLaserPitch{ 1.f };
	constexpr float PlayerLaserLoopStart{ 0.f };
	constexpr float PlayerLaserLoopEnd{ 0.63f };
	constexpr float PlayerLaserOutroStart{ 0.63f };
}

Player::Player(Assets& assets, World& world, InputHandler<Config::PlayerAction>& input,
	GamepadManager& gamepadManager)
	: Entity(assets, world, assets.Textures().Get(Config::Texture::PlayerShip),
		assets.GetGameplayData().GetPlayer().visualScale,
		assets.GetGameplayData().GetPlayer().collisionRadius,
		assets.GetGameplayData().GetPlayer().collisionCircles)
	, input(input)
	, gamepad(gamepadManager)
{
	BindInput();
}

Player::~Player()
{
	StopLaserSounds();
	input.UnsubscribeAll(Config::PlayerAction::Up);
	input.UnsubscribeAll(Config::PlayerAction::Down);
	input.UnsubscribeAll(Config::PlayerAction::Left);
	input.UnsubscribeAll(Config::PlayerAction::Right);
	input.UnsubscribeAll(Config::PlayerAction::Fire);
}

Entity::Type Player::GetType() const noexcept { return Type::Player; }

bool Player::IsCollideWith(const Entity& other) const
{
	return other.GetType() != Type::Projectile_Player && CheckCollision(other);
}

void Player::Update(float deltaTime)
{
	shootTimer += deltaTime;
	UpdateInvulnerability(deltaTime);
	UpdateMovement(deltaTime);
	if (controlEnabled)
		UpdateRotation();
	UpdateLaser(deltaTime);
}

void Player::HandleEvent(const sf::Event& event)
{
	input.HandleEvent(event);
	if (event.is<sf::Event::MouseMoved>() ||
		event.is<sf::Event::MouseButtonPressed>() ||
		event.is<sf::Event::KeyPressed>())
		aimingWithGamepad = false;
}

void Player::HandleRealtime()
{
	if (!controlEnabled)
		return;
	input.Update();
	const GamepadManager::GameplayInput gamepadInput{ gamepad.GetGameplayInput() };
	moveInput += gamepadInput.movement;
	if (gamepadInput.aimDirection)
	{
		gamepadAimDirection = *gamepadInput.aimDirection;
		aimingWithGamepad = true;
	}
	if (gamepadInput.isFiring)
		Shoot();
}

void Player::SetControlEnabled(bool enabled) noexcept
{
	controlEnabled = enabled;
	if (!controlEnabled)
	{
		StopLaserSounds();
		moveInput = { 0.f, 0.f };
		laserRequested = false;
		laserFiring = false;
		laserDamageTimer = 0.f;
	}
}

void Player::OnDestroy()
{
	StopLaserSounds();
	SetVisible(true);
	GetWorld().AddEffectEvent({
		World::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), 1.35f });
}

bool Player::TakeDamage(int damage)
{
	if (cinematicInvulnerable)
		return false;
	if (IsInvulnerable() || !IsAlive())
		return false;

	auto& session{ GetWorld().GetSession() };
	const GameplaySession::PlayerDamageResult result{ session.ApplyPlayerDamage(damage) };
	if (!result.accepted)
		return false;

	lastDamageReachedHealth = result.healthDamaged;
	blinkDuringInvulnerability = result.healthDamaged;
	if (session.GetPlayerHealth().IsDepleted())
	{
		Destroy();
		return true;
	}

	invulnerabilityTimer = GetAssets().GetGameplayData().GetPlayer().damageInvulnerability;
	return true;
}

void Player::SetCinematicInvulnerable(bool enabled) noexcept
{
	cinematicInvulnerable = enabled;
}

bool Player::DidLastDamageReachHealth() const noexcept
{
	return lastDamageReachedHealth;
}

bool Player::IsInvulnerable() const noexcept
{
	return invulnerabilityTimer > 0.f;
}

bool Player::IsThrusting() const noexcept
{
	return isThrusting;
}

std::array<sf::Vector2f, 2> Player::GetEngineEmitterPositions() const
{
	const sf::Sprite& entitySprite{ GetSprite() };
	const sf::IntRect textureRect{ entitySprite.getTextureRect() };
	const auto& emitterConfig{ GetAssets().GetGameplayData().GetPlayer().engineEmitters };
	std::array<sf::Vector2f, 2> result;
	for (std::size_t i{ 0 }; i < result.size(); ++i)
	{
		const sf::Vector2f localPosition{
			static_cast<float>(textureRect.position.x) +
				static_cast<float>(textureRect.size.x) * emitterConfig[i].x,
			static_cast<float>(textureRect.position.y) +
				static_cast<float>(textureRect.size.y) * emitterConfig[i].y };
		result[i] = entitySprite.getTransform().transformPoint(localPosition);
	}
	return result;
}

void Player::SetFiringEnabled(bool enabled) noexcept
{
	firingEnabled = enabled;
	if (!firingEnabled)
	{
		laserRequested = false;
		laserFiring = false;
		laserDamageTimer = 0.f;
		StopLaserSounds();
	}
}

sf::Vector2f Player::GetMuzzlePosition() const
{
	const sf::Sprite& entitySprite{ GetSprite() };
	const sf::IntRect textureRect{ entitySprite.getTextureRect() };
	const auto& emitter{ GetAssets().GetGameplayData().GetPlayer().muzzleEmitter };
	const sf::Vector2f localPosition{
		static_cast<float>(textureRect.position.x) +
			static_cast<float>(textureRect.size.x) * emitter.x,
		static_cast<float>(textureRect.position.y) +
			static_cast<float>(textureRect.size.y) * emitter.y };
	return entitySprite.getTransform().transformPoint(localPosition);
}

sf::Vector2f Player::GetLaserEndPosition() const
{
	const sf::Vector2f start{ GetPosition() };
	const sf::Vector2f direction{ GetAimDirection() };
	float distance{ std::numeric_limits<float>::max() };
	const float width{ static_cast<float>(GetWorld().GetWidth()) };
	const float height{ static_cast<float>(GetWorld().GetHeight()) };
	if (direction.x > 0.0001f)
		distance = std::min(distance, (width - start.x) / direction.x);
	else if (direction.x < -0.0001f)
		distance = std::min(distance, -start.x / direction.x);
	if (direction.y > 0.0001f)
		distance = std::min(distance, (height - start.y) / direction.y);
	else if (direction.y < -0.0001f)
		distance = std::min(distance, -start.y / direction.y);
	return start + direction * std::max(0.f, distance);
}

bool Player::IsLaserFiring() const noexcept { return laserFiring; }
float Player::GetLaserVisualTime() const noexcept { return laserVisualTime; }

sf::Vector2f Player::GetExhaustDirection() const noexcept
{
	const float angle{ GetRotation().asRadians() + std::numbers::pi_v<float> * 0.5f };
	return { std::cos(angle), std::sin(angle) };
}

std::optional<sf::Vector2f> Player::GetGamepadAimPoint() const
{
	if (!aimingWithGamepad)
		return std::nullopt;
	return GetPosition() + gamepadAimDirection * 190.f;
}

void Player::BindInput()
{
	using enum Config::PlayerAction;
	input.Subscribe(Up, [this]() { moveInput.y -= 1.f; });
	input.Subscribe(Down, [this]() { moveInput.y += 1.f; });
	input.Subscribe(Left, [this]() { moveInput.x -= 1.f; });
	input.Subscribe(Right, [this]() { moveInput.x += 1.f; });
	input.Subscribe(Fire, [this]() { Shoot(); });
}

void Player::UpdateMovement(float dt)
{
	const auto& config{ GetAssets().GetGameplayData().GetPlayer() };
	const float speedMultiplier{ GetWorld().GetSession().GetSpeedMultiplier() };
	sf::Vector2f currentVelocity{ GetVelocity() };

	isThrusting = moveInput.x != 0.f || moveInput.y != 0.f;
	if (isThrusting)
	{
		const float length{ std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y) };
		const float intensity{ std::min(length, 1.f) };
		currentVelocity += moveInput / length * config.acceleration * speedMultiplier * intensity * dt;
	}

	const float speed{ std::sqrt(
		currentVelocity.x * currentVelocity.x + currentVelocity.y * currentVelocity.y) };
	const float maximumSpeed{ config.maximumSpeed * speedMultiplier };
	if (speed > maximumSpeed)
		currentVelocity = currentVelocity / speed * maximumSpeed;

	currentVelocity *= std::pow(config.damping, dt * 60.f);
	SetVelocity(currentVelocity);
	Move(dt);
	moveInput = { 0.f, 0.f };
}

void Player::UpdateRotation()
{
	if (aimingWithGamepad)
	{
		SetRotation(sf::radians(std::atan2(gamepadAimDirection.y, gamepadAimDirection.x)
			+ std::numbers::pi_v<float> / 2.f));
		return;
	}

	sf::RenderWindow& window{ GetWorld().GetWindow() };
	const sf::Vector2i mousePixel{ sf::Mouse::getPosition(window) };
	const sf::Vector2f mouseWorld{ window.mapPixelToCoords(mousePixel) };
	const sf::Vector2f toMouse{ mouseWorld - GetPosition() };
	SetRotation(sf::radians(std::atan2(toMouse.y, toMouse.x)
		+ std::numbers::pi_v<float> / 2.f));
}

void Player::UpdateInvulnerability(float dt)
{
	if (invulnerabilityTimer <= 0.f)
	{
		SetVisible(true);
		blinkDuringInvulnerability = false;
		return;
	}

	invulnerabilityTimer = std::max(0.f, invulnerabilityTimer - dt);
	if (invulnerabilityTimer <= 0.f)
	{
		SetVisible(true);
		blinkDuringInvulnerability = false;
		return;
	}

	if (!blinkDuringInvulnerability)
	{
		SetVisible(true);
		return;
	}

	constexpr float BlinkInterval{ 0.1f };
	SetVisible(static_cast<int>(invulnerabilityTimer / BlinkInterval) % 2 == 0);
}

sf::Vector2f Player::GetAimDirection() const noexcept
{
	const float angle{ GetRotation().asRadians() - std::numbers::pi_v<float> * 0.5f };
	return { std::cos(angle), std::sin(angle) };
}

void Player::UpdateLaser(float dt)
{
	const auto& pickupConfig{ GetAssets().GetGameplayData().GetPickups() };
	const bool bonusActive{ GetWorld().GetSession().IsLaserActive() };
	const bool wasFiring{ laserFiring };
	laserFiring = controlEnabled && laserRequested && bonusActive;
	laserRequested = false;
	if (!bonusActive)
	{
		StopLaserSounds();
		laserFiring = false;
		laserDamageTimer = 0.f;
		return;
	}
	if (!laserFiring)
	{
		if (wasFiring)
		{
			// The laser must stop the instant the fire button is released,
			// not fade out through a release/outro tail.
			GetWorld().StopSound(laserSoundHandle);
			laserSoundHandle = 0u;
		}
		laserDamageTimer = 0.f;
		return;
	}

	laserVisualTime += dt;
	laserDamageTimer += dt;
	if (!wasFiring)
	{
		laserDamageTimer = pickupConfig.laserDamageInterval;
		laserSoundHandle = GetWorld().AddSustainedSound(
			Config::Sound::PlayerLaserShot,
			PlayerLaserPitch,
			PlayerLaserLoopStart,
			PlayerLaserLoopEnd,
			PlayerLaserOutroStart);
	}
	while (laserDamageTimer >= pickupConfig.laserDamageInterval)
	{
		laserDamageTimer -= pickupConfig.laserDamageInterval;
		const std::uint64_t attackID{ GetWorld().BeginPlayerAttack() };
		const int damage{ GetAssets().GetGameplayData().GetProjectile(
			GameplayData::ProjectileKind::Player).damage };
		GetWorld().DamageEnemiesWithPlayerLaser(
			GetPosition(), GetLaserEndPosition(),
			pickupConfig.laserWidth, damage, attackID);
	}
}

void Player::StopLaserSounds()
{
	GetWorld().StopSound(laserSoundHandle);
	laserSoundHandle = 0u;
}

void Player::Shoot()
{
	if (!controlEnabled || !firingEnabled)
		return;
	if (GetWorld().GetSession().IsLaserActive())
	{
		laserRequested = true;
		return;
	}
	const float cooldown{ GetAssets().GetGameplayData().GetPlayer().shootCooldown /
		GetWorld().GetSession().GetFireRateMultiplier() };
	if (shootTimer < cooldown)
		return;

	const std::uint64_t attackID{ GetWorld().BeginPlayerAttack() };
	const float rotation{ GetRotation().asDegrees() };
	const bool tripleShot{ GetWorld().GetSession().IsTripleShotActive() };
	GetWorld().SpawnPlayerShot(
		GetMuzzlePosition(), rotation, attackID, true, tripleShot);
	if (tripleShot)
	{
		const float spread{
			GetAssets().GetGameplayData().GetPickups().tripleShotAngleDegrees };
		GetWorld().SpawnPlayerShot(
			GetMuzzlePosition(), rotation - spread, attackID, false, true);
		GetWorld().SpawnPlayerShot(
			GetMuzzlePosition(), rotation + spread, attackID, false, true);
	}
	shootTimer = 0.f;
}
