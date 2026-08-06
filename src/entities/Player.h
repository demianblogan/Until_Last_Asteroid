#pragma once

#include "core/Entity.h"
#include "systems/InputHandler.h"
#include "utils/ConfigEnums.h"

class AssetStore;
class World;

namespace sf
{
	class Event;
}

class Player final : public Entity
{
public:
	Player(AssetStore& assets, World& world, InputHandler<Config::PlayerAction>& input);
	~Player();

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void HandleEvent(const sf::Event& event);
	void HandleRealtime();
	void OnDestroy() override;

private:
	void BindInput();
	void Shoot();

	void UpdateMovement(float dt);
	void UpdateRotation();
	void UpdateSpawnProtection(float dt);
	[[nodiscard]] bool IsSpawnProtected() const noexcept;

	InputHandler<Config::PlayerAction>& input;
	sf::Vector2f moveInput{ 0.f, 0.f };
	float shootTimer{ 0.f };
	float spawnProtectionTimer{ SPAWN_PROTECTION_DURATION };

	static constexpr float ACCELERATION = 1200.f;
	static constexpr float DAMPING = 0.98f;
	static constexpr float MAX_SPEED = 600.f;
	static constexpr float SPAWN_PROTECTION_DURATION = 2.f;
	static constexpr float SPAWN_BLINK_INTERVAL = 0.1f;
};