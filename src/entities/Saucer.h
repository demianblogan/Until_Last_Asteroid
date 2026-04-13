#pragma once

#include "Enemy.h"

class AssetStore;
class World;

namespace sf
{
	class Texture;
}

class Saucer final : public Enemy
{
public:
	enum class Mode
	{
		Kamikaze,
		Shooter
	};

	Saucer(AssetStore& assets, World& world, Mode mode);

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	static float GetSpeed(Mode mode) noexcept;
	static int GetScore(Mode mode) noexcept;
	static sf::Texture& GetTexture(AssetStore& assets, Mode mode);

	void UpdateMovement(float deltaTime, const sf::Vector2f& target);
	void Shoot(const sf::Vector2f& playerPosition);

	Mode mode;
	float shootTimer{ 0.f };
};