#include "KamikazeSaucer.h"

#include <cmath>

#include "assets/Assets.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"

KamikazeSaucer::KamikazeSaucer(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::BigEnemySaucer),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Kamikaze))
{
	spinDirection = Random::Float(0.f, 1.f) < 0.5f ? -1.f : 1.f;
}

void KamikazeSaucer::Update(float deltaTime)
{
	SetRotation(GetRotation() + sf::degrees(GetRotationSpeed() * spinDirection * deltaTime));

	const sf::Vector2f playerPosition{ GetWorld().GetPlayerPosition() };
	const sf::Vector2f toPlayer{ playerPosition - GetPosition() };
	const float angle = std::atan2(toPlayer.y, toPlayer.x);
	const sf::Vector2f direction{ std::cos(angle), std::sin(angle) };

	SetVelocity(direction * GetMovementSpeed());
	Move(deltaTime);
}

void KamikazeSaucer::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPosition(), GetVelocity(), 0.95f });
}
