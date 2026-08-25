#include "HelperBot.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "gameplay/GameplaySession.h"
#include "utils/ConfigEnums.h"

HelperBot::HelperBot(Assets& assets, World& world)
	: Entity(
		assets,
		world,
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

bool HelperBot::IsCollideWith(const Entity& other) const
{
	static_cast<void>(other);
	return false;
}

void HelperBot::Update(float deltaTime)
{
	if (!GetWorld().HasPlayer() || !GetWorld().GetSession().IsHelperBotActive())
	{
		Destroy();
		return;
	}

	const auto& config{ GetAssets().GetGameplayData().GetPickups() };
	const float orbitSpeedRadians{
		config.helperBotOrbitSpeedDegrees * std::numbers::pi_v<float> / 180.f };
	orbitPhaseRadians = std::fmod(
		orbitPhaseRadians + orbitSpeedRadians * deltaTime,
		2.f * std::numbers::pi_v<float>);
	const sf::Vector2f orbitOffset{
		std::cos(orbitPhaseRadians) * config.helperBotOrbitRadius,
		std::sin(orbitPhaseRadians) * config.helperBotOrbitRadius };
	SetPosition(GetWorld().GetPlayerPosition() + orbitOffset);

	shotCooldown = std::max(0.f, shotCooldown - deltaTime);
	const Entity* target{ GetWorld().FindHomingTarget(
		GetPosition(), { 1.f, 0.f }, -1.f) };
	if (target == nullptr)
		return;

	const sf::Vector2f toTarget{ target->GetPosition() - GetPosition() };
	const float targetAngle{ std::atan2(toTarget.y, toTarget.x) };
	SetRotation(sf::radians(targetAngle));
	if (shotCooldown > 0.f)
		return;

	const sf::Vector2f direction{ std::cos(targetAngle), std::sin(targetAngle) };
	GetWorld().SpawnHelperShot(GetPosition() + direction * 32.f, target);
	shotCooldown = config.helperBotShotInterval;
}
