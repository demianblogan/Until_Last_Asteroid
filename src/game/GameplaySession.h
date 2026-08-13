#pragma once

#include "game/Health.h"
#include "game/Shield.h"

class GameplaySession
{
public:
	struct PlayerDamageResult
	{
		bool accepted{ false };
		bool shieldDamaged{ false };
		bool healthDamaged{ false };
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
	[[nodiscard]] int GetLevel() const noexcept;
	[[nodiscard]] int GetScore() const noexcept;

	[[nodiscard]] bool IsPlaying() const noexcept;
	[[nodiscard]] bool IsGameOver() const noexcept;
	[[nodiscard]] bool IsLevelComplete() const noexcept;
	[[nodiscard]] bool IsWin() const noexcept;

	void SetWin() noexcept;

	void SetLevelComplete() noexcept;
	void SetGameOver() noexcept;

	void ConfigurePlayerHealth(int maximumHealth) noexcept;
	void ConfigureShield(float capacity, float duration) noexcept;
	void Update(float deltaTime) noexcept;
	[[nodiscard]] PlayerDamageResult ApplyPlayerDamage(int damage) noexcept;
	[[nodiscard]] bool RestorePlayerHealth(int amount) noexcept;
	void ActivateShield() noexcept;
	void ActivateHomingBullets(float duration) noexcept;
	void ActivateTimeSlowdown(float duration) noexcept;
	void ClearTemporaryEffects() noexcept;
	void Reset() noexcept;
	void StartAtLevel(int levelNumber, int accumulatedScore) noexcept;
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
	int level{ 1 };
	int score{ 0 };
	int levelStartScore{ 0 };
};
