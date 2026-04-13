#pragma once

#include "core/Entity.h"

class AssetStore;
class World;

namespace sf
{
	class Texture;

}
class Enemy : public Entity
{
public:
	Enemy(AssetStore& assets, World& world, sf::Texture& texture, int scoreValue, float speed);

	[[nodiscard]] int GetScoreValue() const noexcept;
	Type GetType() const noexcept override;

protected:
	void Update(float deltaTime) override;

private:
	int scoreValue{ 0 };
};