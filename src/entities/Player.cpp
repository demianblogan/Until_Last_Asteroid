#include "Player.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "gameplay/GameplaySession.h"
#include "input/gamepad/GamepadManager.h"
#include "input/gamepad/GamepadHaptics.h"

namespace
{
	constexpr float PlayerLaserPitch = 1.f;
	constexpr float PlayerLaserLoopStart = 0.f;
	constexpr float PlayerLaserLoopEnd = 0.63f;
	constexpr float PlayerLaserOutroStart = 0.63f;
}

Player::Player(Assets& assets, World& world, InputHandler<Config::PlayerAction>& input,
	GamepadManager& gamepadManager, sf::RenderWindow& gameWindow)
	: Entity(assets, world, assets.Textures().Get(Config::Texture::PlayerShip),
		assets.GetGameplayData().GetPlayer().visualScale,
		assets.GetGameplayData().GetPlayer().collisionRadius,
		assets.GetGameplayData().GetPlayer().collisionCircles)
	, input(input)
	, gamepad(gamepadManager)
	, window(gameWindow)
{
	BindInput();
}

Player::~Player()
{
	StopLaserSounds();
	UnbindInput();
}

Entity::Type Player::GetType() const noexcept
{
	return Type::Player;
}

bool Player::IsCollidingWith(const Entity& other) const
{
	return other.GetType() != Type::Projectile_Player && CheckCollision(other);
}

void Player::Update(float deltaTime)
{
	shootTimer += deltaTime;

	UpdateInvulnerability(deltaTime);
	UpdateMovement(deltaTime);

	if (isControlEnabled)
		UpdateRotation();

	UpdateLaser(deltaTime);
}

void Player::HandleEvent(const sf::Event& event)
{
	input.HandleEvent(event);
	if (event.is<sf::Event::MouseMoved>() ||
		event.is<sf::Event::MouseButtonPressed>() ||
		event.is<sf::Event::KeyPressed>())
	{
		isAimingWithGamepad = false;
	}
}

void Player::HandleRealtime()
{
	if (!isControlEnabled)
		return;

	input.Update();

	const GamepadManager::GameplayInput gamepadInput{ gamepad.GetGameplayInput() };

	moveInput += gamepadInput.movement;

	if (gamepadInput.aimDirection)
	{
		gamepadAimDirection = *gamepadInput.aimDirection;
		isAimingWithGamepad = true;
	}

	if (gamepadInput.isFiring)
		Shoot();
}

void Player::SetControlEnabled(bool isEnabled) noexcept
{
	isControlEnabled = isEnabled;

	if (!isControlEnabled)
	{
		StopLaserSounds();

		moveInput = { 0.f, 0.f };
		isLaserRequested = false;
		isLaserFiring = false;
		laserDamageTimer = 0.f;
	}
}

void Player::OnDestroy()
{
	StopLaserSounds();
	SetVisible(true);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPosition(), GetVelocity(), 1.35f });
}

bool Player::TakeDamage(int damage)
{
	if (isCinematicInvulnerable)
		return false;

	if (IsInvulnerable() || !IsAlive())
		return false;

	auto& session = GetWorld().GetSession();
	const GameplaySession::PlayerDamageResult result{ session.ApplyPlayerDamage(damage) };

	if (!result.wasAccepted)
		return false;

	didLastDamageReachHealth = result.wasHealthDamaged;
	isBlinkingDuringInvulnerability = result.wasHealthDamaged;

	if (session.GetPlayerHealth().IsDepleted())
	{
		Destroy();
		return true;
	}

	invulnerabilityTimer = GetAssets().GetGameplayData().GetPlayer().damageInvulnerability;

	return true;
}

void Player::SetCinematicInvulnerable(bool isEnabled) noexcept
{
	isCinematicInvulnerable = isEnabled;
}

bool Player::DidLastDamageReachHealth() const noexcept
{
	return didLastDamageReachHealth;
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
	const auto& emitterConfig = GetAssets().GetGameplayData().GetPlayer().engineEmitters;
	std::array<sf::Vector2f, 2> result;

	for (std::size_t i = 0; i < result.size(); i++)
		result[i] = TransformNormalizedPoint(emitterConfig[i]);

	return result;
}

void Player::SetFiringEnabled(bool isEnabled) noexcept
{
	isFiringEnabled = isEnabled;

	if (!isFiringEnabled)
	{
		isLaserRequested = false;
		isLaserFiring = false;
		laserDamageTimer = 0.f;

		StopLaserSounds();
	}
}

sf::Vector2f Player::GetMuzzlePosition() const
{
	return TransformNormalizedPoint(GetAssets().GetGameplayData().GetPlayer().muzzleEmitter);
}

sf::Vector2f Player::GetLaserEndPosition() const
{
	const sf::Vector2f start{ GetPosition() };
	const sf::Vector2f direction{ GetAimDirection() };
	float distance = std::numeric_limits<float>::max();
	const float width = static_cast<float>(GetWorld().GetWidth());
	const float height = static_cast<float>(GetWorld().GetHeight());

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

bool Player::IsLaserFiring() const noexcept
{
	return isLaserFiring;
}

float Player::GetLaserVisualTime() const noexcept
{
	return laserVisualTime;
}

sf::Vector2f Player::GetExhaustDirection() const noexcept
{
	// Exhaust fires out the back -- directly opposite where the nose points.
	return -GetForwardDirection();
}

std::optional<sf::Vector2f> Player::GetGamepadAimPoint() const
{
	if (!isAimingWithGamepad)
		return std::nullopt;

	return GetPosition() + gamepadAimDirection * 190.f;
}

std::optional<PlayerEffectState> Player::GetEffectState() const
{
	if (!IsAlive())
		return std::nullopt;

	return PlayerEffectState
	{
		GetEngineEmitterPositions(),
		GetVelocity(),
		GetExhaustDirection(),
		IsThrusting()
	};
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

void Player::UnbindInput()
{
	using enum Config::PlayerAction;

	input.UnsubscribeAll(Up);
	input.UnsubscribeAll(Down);
	input.UnsubscribeAll(Left);
	input.UnsubscribeAll(Right);
	input.UnsubscribeAll(Fire);
}

void Player::UpdateMovement(float deltaTime)
{
	const auto& config = GetAssets().GetGameplayData().GetPlayer();
	const float speedMultiplier = GetWorld().GetSession().GetSpeedMultiplier();
	sf::Vector2f currentVelocity{ GetVelocity() };

	isThrusting = moveInput.x != 0.f || moveInput.y != 0.f;

	if (isThrusting)
	{
		const float length = moveInput.length();
		const float intensity = std::min(length, 1.f);

		currentVelocity += moveInput / length * config.acceleration * speedMultiplier * intensity * deltaTime;
	}

	const float speed = currentVelocity.length();
	const float maximumSpeed = config.maximumSpeed * speedMultiplier;

	if (speed > maximumSpeed)
		currentVelocity = currentVelocity / speed * maximumSpeed;

	currentVelocity *= std::pow(config.damping, deltaTime * 60.f);
	SetVelocity(currentVelocity);
	Move(deltaTime);

	moveInput = { 0.f, 0.f };
}

void Player::UpdateRotation()
{
	// Sprite art points up at rotation 0, so facing an aim vector means its
	// angle turned a further +90 degrees.
	if (isAimingWithGamepad)
	{
		if (gamepadAimDirection.lengthSquared() > 0.0001f)
			SetRotation(gamepadAimDirection.angle() + sf::degrees(90.f));
		return;
	}

	const sf::Vector2i mousePixel{ sf::Mouse::getPosition(window) };
	const sf::Vector2f mouseWorld{ window.mapPixelToCoords(mousePixel) };
	const sf::Vector2f toMouse{ mouseWorld - GetPosition() };

	if (toMouse.lengthSquared() > 0.0001f)
		SetRotation(toMouse.angle() + sf::degrees(90.f));
}

void Player::UpdateInvulnerability(float deltaTime)
{
	if (invulnerabilityTimer <= 0.f)
	{
		SetVisible(true);
		isBlinkingDuringInvulnerability = false;
		return;
	}

	// No std::max(0.f, ...) clamp needed here: the check right below already
	// catches a negative result and returns before anything else reads
	// invulnerabilityTimer, so it's never observed negative regardless.
	invulnerabilityTimer -= deltaTime;

	if (invulnerabilityTimer <= 0.f)
	{
		SetVisible(true);
		isBlinkingDuringInvulnerability = false;
		return;
	}

	if (!isBlinkingDuringInvulnerability)
	{
		SetVisible(true);
		return;
	}

	constexpr float BlinkInterval = 0.1f;
	SetVisible(static_cast<int>(invulnerabilityTimer / BlinkInterval) % 2 == 0);
}

sf::Vector2f Player::GetAimDirection() const noexcept
{
	// The ship aims straight out its nose -- that's exactly "forward".
	return GetForwardDirection();
}

void Player::UpdateLaser(float deltaTime)
{
	const auto& pickupConfig = GetAssets().GetGameplayData().GetPickups();
	const bool isBonusActive = GetWorld().GetSession().IsLaserActive();
	const bool wasFiring = isLaserFiring;

	isLaserFiring = isControlEnabled && isLaserRequested && isBonusActive;
	isLaserRequested = false;

	if (!isBonusActive)
	{
		StopLaserSounds();

		isLaserFiring = false;
		laserDamageTimer = 0.f;

		GetWorld().Haptics().SetRightTriggerSustainedResistance(false);

		return;
	}

	if (!isLaserFiring)
	{
		if (wasFiring)
		{
			// The laser must stop the instant the fire button is released,
			// not fade out through a release/outro tail.
			GetWorld().Sound().StopSound(laserSoundHandle);
			laserSoundHandle = 0u;
		}

		laserDamageTimer = 0.f;
		GetWorld().Haptics().SetRightTriggerSustainedResistance(false);

		return;
	}

	// Heavy, constant resistance on the right trigger for as long as the
	// laser beam is held down; released the instant firing stops (both
	// early-return branches above).
	GetWorld().Haptics().SetRightTriggerSustainedResistance(true);

	laserVisualTime += deltaTime;
	laserDamageTimer += deltaTime;

	if (!wasFiring)
	{
		laserDamageTimer = pickupConfig.laserDamageInterval;
		laserSoundHandle = GetWorld().Sound().AddSustainedSound(
			Config::Sound::PlayerLaserShot,
			PlayerLaserPitch,
			PlayerLaserLoopStart,
			PlayerLaserLoopEnd,
			PlayerLaserOutroStart);
	}

	while (laserDamageTimer >= pickupConfig.laserDamageInterval)
	{
		laserDamageTimer -= pickupConfig.laserDamageInterval;

		const std::uint64_t attackID = GetWorld().BeginPlayerAttack();
		const int damage = GetAssets().GetGameplayData().GetProjectile(GameplayData::ProjectileKind::Player).damage;

		GetWorld().DamageEnemiesWithPlayerLaser(GetPosition(), GetLaserEndPosition(),
			pickupConfig.laserWidth, damage, attackID);
	}
}

void Player::StopLaserSounds()
{
	GetWorld().Sound().StopSound(laserSoundHandle);
	laserSoundHandle = 0u;
}

void Player::Shoot()
{
	if (!isControlEnabled || !isFiringEnabled)
		return;

	if (GetWorld().GetSession().IsLaserActive())
	{
		isLaserRequested = true;
		return;
	}

	const float cooldown =
		GetAssets().GetGameplayData().GetPlayer().shootCooldown / GetWorld().GetSession().GetFireRateMultiplier();

	if (shootTimer < cooldown)
		return;

	const std::uint64_t attackID = GetWorld().BeginPlayerAttack();
	const sf::Vector2f aim{ GetAimDirection() };
	const bool tripleShot = GetWorld().GetSession().IsTripleShotActive();

	GetWorld().Haptics().PulseRightTriggerRecoil();
	GetWorld().SpawnPlayerShot(GetMuzzlePosition(), aim, attackID, true, tripleShot);

	if (tripleShot)
	{
		const sf::Angle spread{ sf::degrees(GetAssets().GetGameplayData().GetPickups().tripleShotAngleDegrees) };
		GetWorld().SpawnPlayerShot(GetMuzzlePosition(), aim.rotatedBy(-spread), attackID, false, true);
		GetWorld().SpawnPlayerShot(GetMuzzlePosition(), aim.rotatedBy(spread), attackID, false, true);
	}

	shootTimer = 0.f;
}