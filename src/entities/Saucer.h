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
	[[nodiscard]] Mode GetMode() const noexcept;
	void BeginMaterialization(float duration, const Entity* anchor = nullptr) noexcept;
	void ConfigureApproachTarget(sf::Vector2f target) noexcept;

	Type GetType() const noexcept override;
	bool IsCollideWith(const Entity& other) const override;
	void Update(float deltaTime) override;
	void OnDestroy() override;

private:
	static const GameplayData::EnemyConfig& GetConfig(AssetStore& assets, Mode mode);
	static sf::Texture& GetTexture(AssetStore& assets, Mode mode);

	void UpdateMovement(float deltaTime, const sf::Vector2f& target);
	[[nodiscard]] bool MoveToTarget(float deltaTime, const sf::Vector2f& target);
	void ChooseCentralPatrolTarget();
	void Shoot(const sf::Vector2f& playerPosition);
	[[nodiscard]] sf::Vector2f GetWeaponEmitterPosition(std::size_t index) const;

	Mode mode;
	float shootTimer{ 0.f };
	float spinDirection{ 1.f };
	std::size_t nextWeaponEmitter{ 0 };
	float materializationRemaining{ 0.f };
	float materializationDuration{ 0.f };
	const Entity* materializationAnchor{ nullptr };
	sf::Vector2f approachTarget;
	sf::Vector2f patrolTarget;
	bool approachingCenter{ false };
	bool hasPatrolTarget{ false };
};
