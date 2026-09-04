#pragma once

#include "gameplay/Health.h"
#include "gameplay/Shield.h"
#include "campaign/ShipUpgrades.h"

#include <string>
#include <unordered_set>
#include <vector>

class GameplaySession
{
public:
	enum class WeaponMode
	{
		Normal,
		Laser,
		TripleShot
	};

	struct PlayerDamageResult
	{
		bool wasAccepted{ false };
		bool wasShieldDamaged{ false };
		bool wasHealthDamaged{ false };
	};

	enum class State
	{
		Playing,
		LevelComplete,
		GameOver,
		Win
	};

	[[nodiscard]] const Health& GetPlayerHealth() const noexcept;
	[[nodiscard]] Health& GetPlayerHealth() noexcept;
	[[nodiscard]] const Shield& GetPlayerShield() const noexcept;
	[[nodiscard]] bool IsHomingBulletsActive() const noexcept;
	[[nodiscard]] float GetHomingBulletsRemaining() const noexcept;
	[[nodiscard]] float GetHomingBulletsRatio() const noexcept;
	[[nodiscard]] bool IsTimeSlowdownActive() const noexcept;
	[[nodiscard]] float GetTimeSlowdownRatio() const noexcept;
	[[nodiscard]] WeaponMode GetWeaponMode() const noexcept;
	[[nodiscard]] bool IsLaserActive() const noexcept;
	[[nodiscard]] bool IsTripleShotActive() const noexcept;
	[[nodiscard]] bool IsHelperBotActive() const noexcept;
	[[nodiscard]] float GetWeaponBonusRatio() const noexcept;
	[[nodiscard]] int GetLevel() const noexcept;
	[[nodiscard]] int GetScore() const noexcept;
	[[nodiscard]] int GetPartsBalance() const noexcept;
	[[nodiscard]] int GetDisplayedParts() const noexcept;
	[[nodiscard]] int GetPartsRecoveredThisLevel() const noexcept;
	[[nodiscard]] float GetArmorMultiplier() const noexcept;
	[[nodiscard]] float GetSpeedMultiplier() const noexcept;
	[[nodiscard]] float GetFireRateMultiplier() const noexcept;
	[[nodiscard]] float GetBonusDurationAddition() const noexcept;
	[[nodiscard]] const ShipUpgradeRanks& GetUpgradeRanks() const noexcept;
	[[nodiscard]] bool HasTakenDamageThisLevel() const noexcept;
	[[nodiscard]] bool IsPartCollected(const std::string& id) const noexcept;
	[[nodiscard]] const std::unordered_set<std::string>& GetCollectedPartIds() const noexcept;

	[[nodiscard]] bool IsPlaying() const noexcept;
	[[nodiscard]] bool IsGameOver() const noexcept;
	[[nodiscard]] bool IsLevelComplete() const noexcept;
	[[nodiscard]] bool IsWin() const noexcept;

	void SetWin() noexcept;

	void SetLevelComplete() noexcept;
	void SetGameOver() noexcept;

	void ConfigurePlayerHealth(int maximumHealth) noexcept;
	void ConfigureShield(float capacity, float duration) noexcept;
	void ConfigureOneHitMode(bool isEnabled) noexcept;
	void ConfigureParts(int balance, const std::vector<std::string>& collectedIds);
	void ConfigureUpgrades(
		const ShipUpgradeRanks& ranks,
		bool needToClampToCampaignMaximum = true) noexcept;
	[[nodiscard]] bool RecoverPart(const std::string& id);
	void AcceptRecoveredParts();
	void DiscardRecoveredParts() noexcept;
#ifdef _DEBUG
	void DebugPreparePartsBalance(int acceptedBalance) noexcept;
#endif
	void Update(float deltaTime) noexcept;
	[[nodiscard]] PlayerDamageResult ApplyPlayerDamage(int damage) noexcept;
	[[nodiscard]] bool RestorePlayerHealth(int amount) noexcept;
	void ActivateShield(float extraDuration = 0.f) noexcept;
	void ActivateHomingBullets(float duration) noexcept;
	void ActivateTimeSlowdown(float duration) noexcept;
	void ActivateLaser(float duration) noexcept;
	void ActivateTripleShot(float duration) noexcept;
	[[nodiscard]] bool ActivateHelperBot() noexcept;
	void ClearTemporaryEffects() noexcept;
	void Reset() noexcept;
	void StartAtLevel(int levelNumber) noexcept;
	void RestartLevel() noexcept;
	void AddScore(int points) noexcept;
	void NextLevel() noexcept;
	[[nodiscard]] int GetLevelScore() const noexcept;

private:
	State state{ State::Playing };
	Health playerHealth;
	Shield playerShield;
	float homingBulletsRemaining{ 0.f };
	float homingBulletsDuration{ 1.f };
	float timeSlowdownRemaining{ 0.f };
	float timeSlowdownDuration{ 1.f };
	WeaponMode weaponMode{ WeaponMode::Normal };
	float weaponBonusRemaining{ 0.f };
	float weaponBonusDuration{ 1.f };
	bool isHelperBotActive{ false };
	int level{ 1 };
	int score{ 0 };
	int levelStartScore{ 0 };
	int partsBalance{ 0 };
	std::unordered_set<std::string> collectedPartIds;
	std::unordered_set<std::string> pendingPartIds;
	ShipUpgradeRanks upgradeRanks;
	bool isOneHitModeEnabled{ false };
	bool hasTakenDamageThisLevel{ false };
};
