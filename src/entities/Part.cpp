#include "Part.h"

#include <algorithm>
#include <cmath>

#include "assets/Assets.h"
#include "gameplay/GameplayData.h"
#include "utils/ConfigEnums.h"

namespace
{
	// How much the token's scale breathes up and down (+/- 8%) as it idles,
	// and how fast (radians/second) -- purely cosmetic "alive" feel.
	constexpr float PulseAmplitude = 0.08f;
	constexpr float PulseFrequency = 7.f;

	// Once inside its final blinkDuration seconds, the token flashes between
	// fully opaque and this dim opacity...
	constexpr float BlinkDimOpacity = 0.2f;
	// ...at this rate (radians/second fed into sin()) -- fast enough to read
	// as an urgent "about to expire" flicker rather than a slow fade.
	constexpr float BlinkFrequency = 22.f;
}

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

	const float pulse = 1.f + PulseAmplitude * std::sin(pulsePhaseElapsed * PulseFrequency);
	float opacity = 1.f;

	if (remainingLifetime < config.blinkDuration)
		opacity = std::sin(remainingLifetime * BlinkFrequency) > 0.f ? 1.f : BlinkDimOpacity;

	SetPresentation(pulse, opacity);
}

bool Part::IsCollidingWith(const Entity& other) const
{
	return other.GetType() == Type::Player;
}