#include "KamikazeSaucer.h"

#include "assets/Assets.h"
#include "core/world/World.h"
#include "utils/ConfigEnums.h"
#include "utils/Random.h"
#include "utils/VectorMath.h"

KamikazeSaucer::KamikazeSaucer(Assets& assets, World& world)
	: Enemy(assets, world, assets.Textures().Get(Config::Texture::BigEnemySaucer),
		assets.GetGameplayData().GetEnemy(GameplayData::EnemyKind::Kamikaze))
{
	spinDirection = Random::Sign();
}

void KamikazeSaucer::Update(float deltaTime)
{
	SetRotation(GetRotation() + sf::degrees(GetRotationSpeed() * spinDirection * deltaTime));

	// Charges straight at the player's current position; the cosmetic spin
	// above is separate from which way it's actually flying.
	const sf::Vector2f toPlayer{ GetWorld().GetPlayerPosition() - GetPosition() };
	SetVelocity(VectorMath::Normalize(toPlayer) * GetMovementSpeed());
	Move(deltaTime);
}

void KamikazeSaucer::OnDestroy()
{
	GetWorld().Sound().AddSound(Config::Sound::ShipExplosion);
	GetWorld().Effects().Add({ Rendering::EffectEventType::ShipExplosion, GetPosition(), GetVelocity(), 0.95f });
}
