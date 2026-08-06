#include "Player.h"

#include <cmath>
#include <numbers>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Mouse.hpp>
#include "assets/AssetStore.h"
#include "core/World.h"

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
}

Entity::Type Player::GetType() const noexcept
{
	return Type::Player;
}

bool Player::IsCollideWith(const Entity& other) const
{
	if (other.GetType() == Type::Projectile_Player)
		return false;

	return CheckCollision(other);
}

void Player::Update(float deltaTime)
{
	shootTimer += deltaTime;

	UpdateMovement(deltaTime);
	UpdateRotation();
}

void Player::HandleEvent(const sf::Event& event)
{
	input.HandleEvent(event);
}

void Player::HandleRealtime()
{
	input.Update();

	if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
		Shoot();
}

void Player::OnDestroy()
{
	GetWorld().AddSound(Config::Sound::PlayerShipExplosion);
}

void Player::BindInput()
{
	using enum Config::PlayerAction;

	input.Subscribe(Up, [this]() { moveInput.y -= 1.f; });
	input.Subscribe(Down, [this]() { moveInput.y += 1.f; });
	input.Subscribe(Left, [this]() { moveInput.x -= 1.f; });
	input.Subscribe(Right, [this]() { moveInput.x += 1.f; });
}

// --------------------------------------------------------
// MOVEMENT
// --------------------------------------------------------
void Player::UpdateMovement(float dt)
{
	sf::Vector2f velocity{ GetVelocity() };

	if (moveInput.x != 0.f || moveInput.y != 0.f)
	{
		float length{ std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y) };
		sf::Vector2f direction{ moveInput / length };

		velocity += direction * ACCELERATION * dt;
	}

	float speed{ std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y) };
	if (speed > MAX_SPEED)
		velocity = velocity / speed * MAX_SPEED;

	float frameDamping{ std::pow(DAMPING, dt * 60.f) };
	velocity *= frameDamping;

	SetVelocity(velocity);
	Move(dt);

	moveInput = { 0.f, 0.f };
}

void Player::UpdateRotation()
{
	sf::RenderWindow& window{ GetWorld().GetWindow() };
	sf::Vector2i mousePixel{ sf::Mouse::getPosition(window) };
	sf::Vector2f mouseWorld{ window.mapPixelToCoords(mousePixel) };
	sf::Vector2f toMouse{ mouseWorld - GetPosition() };
	float angle{ std::atan2(toMouse.y, toMouse.x) };

	SetRotation(sf::radians(angle + std::numbers::pi_v<float> / 2.f));
}

void Player::Shoot()
{
	static constexpr float SHOOT_COOLDOWN{ 0.2f };
	if (shootTimer < SHOOT_COOLDOWN)
		return;

	GetWorld().SpawnPlayerShot(GetPosition(), GetRotation().asDegrees());
	shootTimer = 0.f;
}