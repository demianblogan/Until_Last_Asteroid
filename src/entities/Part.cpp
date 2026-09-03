#include "Part.h"

#include <algorithm>
#include <cmath>

#include "assets/Assets.h"
#include "gameplay/GameplayData.h"
#include "utils/ConfigEnums.h"

Part::Part(Assets& assets, World& world, std::string id)
	: Entity(assets, world, assets.Textures().Get(Config::Texture::PartToken),
		assets.GetGameplayData().GetParts().visualScale,
		assets.GetGameplayData().GetParts().collisionRadius)
	, id(std::move(id))
	, remainingLifetime(assets.GetGameplayData().GetParts().lifetime)
{
	SetPresentation(1.f, 1.f);
}

const std::string& Part::GetID() const noexcept
{
	return id;
}

Entity::Type Part::GetType() const noexcept
{
	return Type::Part;
}

void Part::Update(float deltaTime)
{
	const auto& config = GetAssets().GetGameplayData().GetParts();

	remainingLifetime -= deltaTime;
	pulsePhaseElapsed += deltaTime;

	if (remainingLifetime <= 0.f)
	{
		Destroy();
		return;
	}

	SetRotation(GetRotation() + sf::degrees(config.rotationSpeedDegrees * deltaTime));

	const float pulse = 1.f + 0.08f * std::sin(pulsePhaseElapsed * 7.f);
	float opacity = 1.f;

	if (remainingLifetime < config.blinkDuration)
		opacity = std::sin(remainingLifetime * 22.f) > 0.f ? 1.f : 0.2f;

	SetPresentation(pulse, opacity);
}

bool Part::IsCollidingWith(const Entity& other) const
{
	return other.GetType() == Type::Player;
}