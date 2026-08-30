#pragma once

#include "Enemy.h"

class Assets;
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
		Big
	};

	Meteor(Assets& assets, World& world, Size size);
	[[nodiscard]] Size GetSize() const noexcept;
	void SetFragmentSpawningEnabled(bool enabled) noexcept;

	Type GetType() const noexcept override;

	bool IsCollidingWith(const Entity& other) const override;
	void OnDestroy() override;

private:
	void Update(float deltaTime) override;
	static const GameplayData::EnemyConfig& GetConfig(Assets& assets, Size size);
	static sf::Texture& GetRandomTexture(Assets& assets, Size size);

	Size size;
	bool fragmentSpawningEnabled{ true };
	float angularVelocity{ 0.f };
};
