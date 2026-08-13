#include "Player.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include "assets/AssetStore.h"
#include "core/World.h"
#include "game/GameplaySession.h"
#include "systems/GamepadManager.h"

namespace
{
	constexpr float PlayerLaserPitch{ 1.25f };
	constexpr float PlayerLaserLoopStart{ 1.f };
	constexpr float PlayerLaserLoopEnd{ 4.8f };
	constexpr float PlayerLaserOutroStart{ 4.8f };
}

Player::Player(AssetStore& assets, World& world, InputHandler<Config::PlayerAction>& input,
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
	GetWorld().StopSound(laserSoundHandle);
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
	if (gamepadInput.fire)
		Shoot();
}

void Player::SetControlEnabled(bool enabled) noexcept
{
	controlEnabled = enabled;
	if (!controlEnabled)
	{
		GetWorld().StopSound(laserSoundHandle);
		laserSoundHandle = 0u;
		moveInput = { 0.f, 0.f };
		laserRequested = false;
		laserFiring = false;
		laserDamageTimer = 0.f;
	}
}

void Player::OnDestroy()
{
	GetWorld().StopSound(laserSoundHandle);
	laserSoundHandle = 0u;
	SetVisible(true);
	GetWorld().AddEffectEvent({
		World::EffectEventType::ShipExplosion,
		GetPosition(), GetVelocity(), 1.35f });
}

bool Player::TakeDamage(int damage)
{
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
	const sf::Sprite& sprite{ GetSprite() };
	const sf::IntRect textureRect{ sprite.getTextureRect() };
	const auto& emitterConfig{ GetAssets().GetGameplayData().GetPlayer().engineEmitters };
	std::array<sf::Vector2f, 2> result;
	for (std::size_t i{ 0 }; i < result.size(); ++i)
	{
		const sf::Vector2f localPosition{
			static_cast<float>(textureRect.position.x) +
				static_cast<float>(textureRect.size.x) * emitterConfig[i].x,
			static_cast<float>(textureRect.position.y) +
				static_cast<float>(textureRect.size.y) * emitterConfig[i].y };
		result[i] = sprite.getTransform().transformPoint(localPosition);
	}
	return result;
}

sf::Vector2f Player::GetMuzzlePosition() const
{
	const sf::Sprite& sprite{ GetSprite() };
	const sf::IntRect textureRect{ sprite.getTextureRect() };
	const auto& emitter{ GetAssets().GetGameplayData().GetPlayer().muzzleEmitter };
	const sf::Vector2f localPosition{
		static_cast<float>(textureRect.position.x) +
			static_cast<float>(textureRect.size.x) * emitter.x,
		static_cast<float>(textureRect.position.y) +
			static_cast<float>(textureRect.size.y) * emitter.y };
	return sprite.getTransform().transformPoint(localPosition);
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
	sf::Vector2f velocity{ GetVelocity() };

	isThrusting = moveInput.x != 0.f || moveInput.y != 0.f;
	if (isThrusting)
	{
		const float length{ std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y) };
		const float intensity{ std::min(length, 1.f) };
		velocity += moveInput / length * config.acceleration * speedMultiplier * intensity * dt;
	}

	const float speed{ std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y) };
	const float maximumSpeed{ config.maximumSpeed * speedMultiplier };
	if (speed > maximumSpeed)
		velocity = velocity / speed * maximumSpeed;

	velocity *= std::pow(config.damping, dt * 60.f);
	SetVelocity(velocity);
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
	const bool wasFiring{ laserFiring };
	laserFiring = controlEnabled && laserRequested &&
		GetWorld().GetSession().IsLaserActive();
	laserRequested = false;
	if (!laserFiring)
	{
		if (wasFiring)
		{
			if (controlEnabled && GetWorld().GetSession().IsLaserActive())
				GetWorld().ReleaseSound(laserSoundHandle);
			else
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
			Config::Sound::EnemyLaserShot,
			PlayerLaserPitch,
			PlayerLaserLoopStart,
			PlayerLaserLoopEnd,
			PlayerLaserOutroStart);
	}
	while (laserDamageTimer >= pickupConfig.laserDamageInterval)
	{
		laserDamageTimer -= pickupConfig.laserDamageInterval;
		const std::uint64_t attackId{ GetWorld().BeginPlayerAttack() };
		const int damage{ GetAssets().GetGameplayData().GetProjectile(
			GameplayData::ProjectileKind::Player).damage };
		GetWorld().DamageEnemiesWithPlayerLaser(
			GetPosition(), GetLaserEndPosition(),
			pickupConfig.laserWidth, damage, attackId);
	}
}

void Player::Shoot()
{
	if (!controlEnabled)
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

	const std::uint64_t attackId{ GetWorld().BeginPlayerAttack() };
	const float rotation{ GetRotation().asDegrees() };
	const bool tripleShot{ GetWorld().GetSession().IsTripleShotActive() };
	GetWorld().SpawnPlayerShot(
		GetMuzzlePosition(), rotation, attackId, true, tripleShot);
	if (tripleShot)
	{
		const float spread{
			GetAssets().GetGameplayData().GetPickups().tripleShotAngleDegrees };
		GetWorld().SpawnPlayerShot(
			GetMuzzlePosition(), rotation - spread, attackId, false, true);
		GetWorld().SpawnPlayerShot(
			GetMuzzlePosition(), rotation + spread, attackId, false, true);
	}
	shootTimer = 0.f;
}
