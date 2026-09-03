#pragma once

#include "Enemy.h"

class Assets;
class World;

// Dives straight at the player in a beeline, spinning as it goes. Never
// shoots and never patrols -- if it doesn't hit anything first, it flies
// off the far side of the screen.
class KamikazeSaucer final : public Enemy
{
public:
	KamikazeSaucer(Assets& assets, World& world);

	void OnDestroy() override;

private:
	void Update(float deltaTime) override;

	float spinDirection = 1.f;
};
