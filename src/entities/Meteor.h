#pragma once

#include "Enemy.h"

class AssetStore;
class World;

namespace sf
{
	class Texture;
}

class Meteor final : public Enemy
{
public:
	enum class Size
	{
		Small,
		Medium,
		Big
	};

	Meteor(AssetStore& assets, World& world, Size size);

	[[nodiscard]] Size GetSize() const noexcept;
	Type GetType() const noexcept override;

	bool IsCollideWith(const Entity& other) const override;
	void OnDestroy() override;

private:
	static float GetSpeed(Size size) noexcept;
	static int GetScore(Size size) noexcept;
	static sf::Texture& GetRandomTexture(AssetStore& assets, Size size);

	Size size;
};