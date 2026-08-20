#pragma once

#include <array>
#include <cstdint>
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
	void SetControlEnabled(bool enabled) noexcept;
	void SetFiringEnabled(bool enabled) noexcept;
	void OnDestroy() override;

	[[nodiscard]] bool TakeDamage(int damage);
	[[nodiscard]] bool DidLastDamageReachHealth() const noexcept;
	[[nodiscard]] bool IsInvulnerable() const noexcept;
	[[nodiscard]] bool IsThrusting() const noexcept;
	[[nodiscard]] std::array<sf::Vector2f, 2> GetEngineEmitterPositions() const;
	[[nodiscard]] sf::Vector2f GetMuzzlePosition() const;
	[[nodiscard]] sf::Vector2f GetLaserEndPosition() const;
	[[nodiscard]] bool IsLaserFiring() const noexcept;
	[[nodiscard]] float GetLaserVisualTime() const noexcept;
	[[nodiscard]] sf::Vector2f GetExhaustDirection() const noexcept;
	[[nodiscard]] std::optional<sf::Vector2f> GetGamepadAimPoint() const;

private:
	void BindInput();
	void Shoot();
	void UpdateMovement(float dt);
	void UpdateRotation();
	void UpdateInvulnerability(float dt);
	void UpdateLaser(float dt);
	void StopLaserSounds();
	[[nodiscard]] sf::Vector2f GetAimDirection() const noexcept;

	InputHandler<Config::PlayerAction>& input;
	GamepadManager& gamepad;
	sf::Vector2f moveInput{ 0.f, 0.f };
	sf::Vector2f gamepadAimDirection{ 0.f, -1.f };
	float shootTimer{ 0.f };
	float invulnerabilityTimer{ 0.f };
	float laserDamageTimer{ 0.f };
	float laserVisualTime{ 0.f };
	std::uint64_t laserSoundHandle{ 0u };
	std::uint64_t laserOutroSoundHandle{ 0u };
	bool laserRequested{ false };
	bool laserFiring{ false };
	bool blinkDuringInvulnerability{ false };
	bool lastDamageReachedHealth{ false };
	bool isThrusting{ false };
	bool aimingWithGamepad{ false };
	bool controlEnabled{ true };
	bool firingEnabled{ true };
};
