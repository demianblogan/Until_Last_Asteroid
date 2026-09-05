#include "GameplaySession.h"

#include <algorithm>
#include <utility>

const Health& GameplaySession::GetPlayerHealth() const noexcept
{
	return playerHealth;
}

Health& GameplaySession::GetPlayerHealth() noexcept
{
	return playerHealth;
}

int GameplaySession::GetLevel() const noexcept
{
	return level;
}

int GameplaySession::GetScore() const noexcept
{
	return score;
}

int GameplaySession::GetPartsBalance() const noexcept
{
	return partsBalance;
}

int GameplaySession::GetDisplayedParts() const noexcept
{
	return static_cast<int>(pendingPartIds.size());
}

float GameplaySession::GetArmorMultiplier() const noexcept
{
	return 1.f + ShipUpgradeRules::ArmorPerRank * upgradeRanks.armor;
}

float GameplaySession::GetSpeedMultiplier() const noexcept
{
	return 1.f + ShipUpgradeRules::SpeedPerRank * upgradeRanks.engines;
}

float GameplaySession::GetFireRateMultiplier() const noexcept
{
	return 1.f + ShipUpgradeRules::FireRatePerRank * upgradeRanks.fireRate;
}

float GameplaySession::GetBonusDurationAddition() const noexcept
{
	return ShipUpgradeRules::BonusSecondsPerRank * upgradeRanks.bonusDuration;
}

const ShipUpgradeRanks& GameplaySession::GetUpgradeRanks() const noexcept
{
	return upgradeRanks;
}

bool GameplaySession::HasTakenDamageThisLevel() const noexcept
{
	return hasTakenDamageThisLevel;
}

bool GameplaySession::IsPartCollected(const std::string& id) const noexcept
{
	return collectedPartIds.contains(id) || pendingPartIds.contains(id);
}

const std::unordered_set<std::string>& GameplaySession::GetCollectedPartIds() const noexcept
{
	return collectedPartIds;
}

bool GameplaySession::IsPlaying() const noexcept
{
	return state == State::Playing;
}

bool GameplaySession::IsGameOver() const noexcept
{
	return state == State::GameOver;
}

void GameplaySession::SetWin() noexcept
{
	state = State::Win;
}

void GameplaySession::SetLevelComplete() noexcept
{
	state = State::LevelComplete;
}

void GameplaySession::SetGameOver() noexcept
{
	state = State::GameOver;
}

const Shield& GameplaySession::GetPlayerShield() const noexcept
{
	return playerShield;
}

bool GameplaySession::IsHomingBulletsActive() const noexcept
{
	return homingBulletsRemaining > 0.f;
}

float GameplaySession::GetHomingBulletsRatio() const noexcept
{
	return homingBulletsDuration > 0.f ? homingBulletsRemaining / homingBulletsDuration : 0.f;
}

bool GameplaySession::IsTimeSlowdownActive() const noexcept
{
	return timeSlowdownRemaining > 0.f;
}

float GameplaySession::GetTimeSlowdownRatio() const noexcept
{
	return timeSlowdownDuration > 0.f ? timeSlowdownRemaining / timeSlowdownDuration : 0.f;
}

void GameplaySession::ConfigurePlayerHealth(int maximumHealth) noexcept
{
	playerHealth.SetMaximum(maximumHealth);
}

void GameplaySession::ConfigureShield(float capacity, float duration) noexcept
{
	playerShield.Configure(capacity, duration);
}

void GameplaySession::ConfigureOneHitMode(bool isEnabled) noexcept
{
	isOneHitModeEnabled = isEnabled;
}

GameplaySession::WeaponMode GameplaySession::GetWeaponMode() const noexcept
{
	return weaponBonusRemaining > 0.f ? weaponMode : WeaponMode::Normal;
}

bool GameplaySession::IsLaserActive() const noexcept
{
	return GetWeaponMode() == WeaponMode::Laser;
}

bool GameplaySession::IsTripleShotActive() const noexcept
{
	return GetWeaponMode() == WeaponMode::TripleShot;
}

bool GameplaySession::IsHelperBotActive() const noexcept
{
	return isHelperBotActive;
}

float GameplaySession::GetWeaponBonusRatio() const noexcept
{
	return weaponBonusDuration > 0.f ? weaponBonusRemaining / weaponBonusDuration : 0.f;
}

void GameplaySession::ConfigureParts(int balance, const std::vector<std::string>& collectedIds)
{
	partsBalance = std::max(0, balance);
	collectedPartIds.clear();
	collectedPartIds.insert(collectedIds.begin(), collectedIds.end());
	pendingPartIds.clear();
}

void GameplaySession::ConfigureUpgrades(const ShipUpgradeRanks& ranks, bool needToClampToCampaignMaximum) noexcept
{
	upgradeRanks = ranks;

	if (needToClampToCampaignMaximum)
	{
		ShipUpgradeRules::Clamp(upgradeRanks);
	}
	else
	{
		upgradeRanks.armor = std::max(0, upgradeRanks.armor);
		upgradeRanks.engines = std::max(0, upgradeRanks.engines);
		upgradeRanks.fireRate = std::max(0, upgradeRanks.fireRate);
		upgradeRanks.bonusDuration = std::max(0, upgradeRanks.bonusDuration);
	}
}

bool GameplaySession::RecoverPart(const std::string& id)
{
	if (id.empty() || IsPartCollected(id))
		return false;
	else
		return pendingPartIds.insert(id).second;
}

void GameplaySession::AcceptRecoveredParts()
{
	partsBalance += static_cast<int>(pendingPartIds.size());
	collectedPartIds.insert(pendingPartIds.begin(), pendingPartIds.end());

	pendingPartIds.clear();
}

void GameplaySession::DiscardRecoveredParts() noexcept
{
	pendingPartIds.clear();
}

void GameplaySession::Update(float deltaTime) noexcept
{
	playerShield.Update(deltaTime);

	homingBulletsRemaining = std::max(0.f, homingBulletsRemaining - deltaTime);
	timeSlowdownRemaining = std::max(0.f, timeSlowdownRemaining - deltaTime);
	weaponBonusRemaining = std::max(0.f, weaponBonusRemaining - deltaTime);

	if (weaponBonusRemaining <= 0.f)
		weaponMode = WeaponMode::Normal;
}

GameplaySession::PlayerDamageResult GameplaySession::ApplyPlayerDamage(int damage) noexcept
{
	if (damage <= 0)
		return {};

	const float shieldBefore = playerShield.GetCurrent();
	const int remainingDamage = playerShield.AbsorbDamage(damage);
	const bool wasShieldDamaged = playerShield.GetCurrent() < shieldBefore;
	bool wasHealthDamaged = false;

	if (remainingDamage > 0)
		wasHealthDamaged = playerHealth.ApplyDamage(isOneHitModeEnabled ? playerHealth.GetCurrent() : remainingDamage);

	const bool wasAccepted = wasShieldDamaged || wasHealthDamaged;
	hasTakenDamageThisLevel = hasTakenDamageThisLevel || wasAccepted;

	return { wasAccepted, wasShieldDamaged, wasHealthDamaged };
}

bool GameplaySession::RestorePlayerHealth(int amount) noexcept
{
	return playerHealth.Restore(amount);
}

void GameplaySession::ActivateShield(float extraDuration) noexcept
{
	playerShield.Activate(extraDuration);
}

void GameplaySession::ActivateHomingBullets(float duration) noexcept
{
	homingBulletsDuration = std::max(0.1f, duration);
	homingBulletsRemaining = homingBulletsDuration;
}

void GameplaySession::ActivateTimeSlowdown(float duration) noexcept
{
	timeSlowdownDuration = std::max(0.1f, duration);
	timeSlowdownRemaining = timeSlowdownDuration;
}

void GameplaySession::ActivateLaser(float duration) noexcept
{
	weaponMode = WeaponMode::Laser;
	weaponBonusDuration = std::max(0.1f, duration);
	weaponBonusRemaining = weaponBonusDuration;
}

void GameplaySession::ActivateTripleShot(float duration) noexcept
{
	weaponMode = WeaponMode::TripleShot;
	weaponBonusDuration = std::max(0.1f, duration);
	weaponBonusRemaining = weaponBonusDuration;
}

bool GameplaySession::ActivateHelperBot() noexcept
{
	// Returns whether this call is what turned the bot on -- the pickup is only
	// consumed on the transition, same contract as RestorePlayerHealth().
	return !std::exchange(isHelperBotActive, true);
}

void GameplaySession::ClearTemporaryEffects() noexcept
{
	playerShield.Deactivate();

	homingBulletsRemaining = 0.f;
	timeSlowdownRemaining = 0.f;
	weaponMode = WeaponMode::Normal;
	weaponBonusRemaining = 0.f;
	isHelperBotActive = false;
}

void GameplaySession::Reset() noexcept
{
	playerHealth.Reset();

	ClearTemporaryEffects();
	level = 1;
	score = 0;
	levelStartScore = 0;
	partsBalance = 0;
	collectedPartIds.clear();
	pendingPartIds.clear();
	state = State::Playing;
	hasTakenDamageThisLevel = false;
}

void GameplaySession::StartAtLevel(int levelNumber) noexcept
{
	DiscardRecoveredParts();
	playerHealth.Reset();
	ClearTemporaryEffects();
	level = levelNumber;
	score = 0;
	levelStartScore = 0;
	state = State::Playing;
	hasTakenDamageThisLevel = false;
}

void GameplaySession::RestartLevel() noexcept
{
	DiscardRecoveredParts();
	playerHealth.Reset();
	ClearTemporaryEffects();
	score = levelStartScore;
	state = State::Playing;
	hasTakenDamageThisLevel = false;
}

void GameplaySession::AddScore(int points) noexcept
{
	score += points;
}

void GameplaySession::NextLevel() noexcept
{
	DiscardRecoveredParts();
	playerHealth.Reset();
	ClearTemporaryEffects();
	score = 0;
	levelStartScore = 0;
	level++;
	state = State::Playing;
	hasTakenDamageThisLevel = false;
}

int GameplaySession::GetLevelScore() const noexcept
{
	return score - levelStartScore;
}