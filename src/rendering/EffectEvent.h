#pragma once

#include <SFML/System/Vector2.hpp>

// One-shot visual-effect request that World raises while resolving gameplay
// (a hit, an explosion, a muzzle flash...) and GameplayEffects later turns
// into the actual particles/flashes/shake on screen. World only produces
// these; it never interprets them -- that's why the vocabulary lives here,
// next to the rendering code that's the sole consumer, rather than inside
// World itself.
enum class EffectEventType
{
	PlayerProjectileGlow,
	PlayerHomingProjectileGlow,
	PlayerTripleProjectileGlow,
	EnemyProjectileGlow,
	MissileSmoke,
	EnemyEngine,
	StationWelding,
	StationChainExplosion,
	PlayerMuzzleFlash,
	EnemyMuzzleFlash,
	AsteroidHit,
	ShipHit,
	PlayerHit,
	AsteroidExplosion,
	ShipExplosion,
	StationExplosion,
	PlayerTeleport,
	BossDestructionShake,
	ScorePopup
};

struct EffectEvent
{
	EffectEventType type;
	sf::Vector2f position;
	sf::Vector2f direction;
	float scale{ 1.f };
	int value{ 0 };
};
