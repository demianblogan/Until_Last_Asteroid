#include "HelperBot.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "gameplay/GameplaySession.h"
#include "utils/ConfigEnums.h"
#include "utils/VectorMath.h"

HelperBot::HelperBot(Assets& assets, World& world)
	: Entity(assets, world,
		assets.Textures().Get(Config::Texture::HelperBotPickup),
		assets.GetGameplayData().GetPickups().helperBotVisualScale,
		1.f)
{
	orbitPhaseRadians = std::numbers::pi_v<float>;
	SetPosition(world.GetPlayerPosition());
}

Entity::Type HelperBot::GetType() const noexcept
{
	return Type::Companion;
}

bool HelperBot::IsCollidingWith(const Entity&) const
{
	return false;
}

void HelperBot::Update(float deltaTime)
{
	if (!GetWorld().HasPlayer() || !GetWorld().GetSession().IsHelperBotActive())
	{
		Destroy();
		return;
	}

	const auto& config = GetAssets().GetGameplayData().GetPickups();
	const float orbitSpeedRadians =
		sf::degrees(config.helperBotOrbitSpeedDegrees).asRadians();

	orbitPhaseRadians =
		std::fmod(orbitPhaseRadians + orbitSpeedRadians * deltaTime, 2.f * std::numbers::pi_v<float>);

	// Polar -> cartesian: a point orbitRadius units out at angle orbitPhase.
	const sf::Vector2f orbitOffset{ config.helperBotOrbitRadius, sf::radians(orbitPhaseRadians) };

	SetPosition(GetWorld().GetPlayerPosition() + orbitOffset);

	shotCooldown = std::max(0.f, shotCooldown - deltaTime);

	const Entity* target = GetWorld().FindHomingTarget(GetPosition(), { 1.f, 0.f }, -1.f);
	if (target == nullptr)
		return;

	const sf::Vector2f direction{ VectorMath::Normalize(target->GetPosition() - GetPosition()) };

	// HelperBot's sprite art already points along its firing direction (no
	// +90 nose offset like the ships), so its facing is the raw aim angle.
	SetRotation(direction.angle());

	if (shotCooldown > 0.f)
		return;

	GetWorld().SpawnHelperShot(GetPosition() + direction * 32.f, target);
	shotCooldown = config.helperBotShotInterval;
}