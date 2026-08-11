#pragma once

#include <array>
#include <optional>

#include "core/Entity.h"
#include "systems/InputHandler.h"
#include "utils/ConfigEnums.h"

class AssetStore;
class World;
class GamepadManager;

namespace sf
{
	class Event;
}

class Player final : public Entity
{
public:
	Player(AssetStore& assets, World& world, InputHandler<Config::PlayerAction>& input,
		GamepadManager& gamepad);
	~Player();

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void HandleEvent(const sf::Event& event);
	void HandleRealtime();
	void OnDestroy() override;

	[[nodiscard]] bool TakeDamage(int damage);
	[[nodiscard]] bool DidLastDamageReachHealth() const noexcept;
	[[nodiscard]] bool IsInvulnerable() const noexcept;
	[[nodiscard]] bool IsThrusting() const noexcept;
	[[nodiscard]] std::array<sf::Vector2f, 2> GetEngineEmitterPositions() const;
	[[nodiscard]] sf::Vector2f GetMuzzlePosition() const;
	[[nodiscard]] sf::Vector2f GetExhaustDirection() const noexcept;
	[[nodiscard]] std::optional<sf::Vector2f> GetGamepadAimPoint() const;

private:
	void BindInput();
	void Shoot();
	void UpdateMovement(float dt);
	void UpdateRotation();
	void UpdateInvulnerability(float dt);

	InputHandler<Config::PlayerAction>& input;
	GamepadManager& gamepad;
	sf::Vector2f moveInput{ 0.f, 0.f };
	sf::Vector2f gamepadAimDirection{ 0.f, -1.f };
	float shootTimer{ 0.f };
	float invulnerabilityTimer{ 0.f };
	bool blinkDuringInvulnerability{ false };
	bool lastDamageReachedHealth{ false };
	bool isThrusting{ false };
	bool aimingWithGamepad{ false };
};
