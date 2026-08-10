#include "Player.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include "assets/AssetStore.h"
#include "core/World.h"
#include "game/GameplaySession.h"

Player::Player(AssetStore& assets, World& world, InputHandler<Config::PlayerAction>& input)
	: Entity(assets, world, assets.Textures().Get(Config::Texture::PlayerShip))
	, input(input)
{
	BindInput();
}

Player::~Player()
{
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
	UpdateRotation();
}

void Player::HandleEvent(const sf::Event& event) { input.HandleEvent(event); }
void Player::HandleRealtime() { input.Update(); }

void Player::OnDestroy()
{
	SetVisible(true);
	GetWorld().AddSound(Config::Sound::ShipExplosion);
}

bool Player::TakeDamage(int damage)
{
	if (IsInvulnerable() || !IsAlive())
		return false;

	auto& health{ GetWorld().GetSession().GetPlayerHealth() };
	const bool damageApplied{ health.ApplyDamage(damage) };
	if (!damageApplied)
		return false;
	if (health.IsDepleted())
	{
		Destroy();
		return true;
	}

	invulnerabilityTimer = GetAssets().GetGameplayData().GetPlayer().damageInvulnerability;
	return true;
}

bool Player::IsInvulnerable() const noexcept
{
	return invulnerabilityTimer > 0.f;
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
	sf::Vector2f velocity{ GetVelocity() };

	if (moveInput.x != 0.f || moveInput.y != 0.f)
	{
		const float length{ std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y) };
		velocity += moveInput / length * config.acceleration * dt;
	}

	const float speed{ std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y) };
	if (speed > config.maximumSpeed)
		velocity = velocity / speed * config.maximumSpeed;

	velocity *= std::pow(config.damping, dt * 60.f);
	SetVelocity(velocity);
	Move(dt);
	moveInput = { 0.f, 0.f };
}

void Player::UpdateRotation()
{
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
		return;
	}

	invulnerabilityTimer = std::max(0.f, invulnerabilityTimer - dt);
	if (invulnerabilityTimer <= 0.f)
	{
		SetVisible(true);
		return;
	}

	constexpr float BlinkInterval{ 0.1f };
	SetVisible(static_cast<int>(invulnerabilityTimer / BlinkInterval) % 2 == 0);
}

void Player::Shoot()
{
	const float cooldown{ GetAssets().GetGameplayData().GetPlayer().shootCooldown };
	if (shootTimer < cooldown)
		return;

	GetWorld().SpawnPlayerShot(GetPosition(), GetRotation().asDegrees());
	shootTimer = 0.f;
}
