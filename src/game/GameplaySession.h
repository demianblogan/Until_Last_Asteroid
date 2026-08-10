#pragma once

#include "game/Health.h"

class GameplaySession
{
public:
	enum class State
	{
		Playing,
		LevelComplete,
		GameOver,
		Win
	};

	[[nodiscard]] const Health& GetPlayerHealth() const noexcept;
	[[nodiscard]] Health& GetPlayerHealth() noexcept;
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
	void Reset() noexcept;
	void AddScore(int points) noexcept;
	void NextLevel() noexcept;

private:
	State state{ State::Playing };
	Health playerHealth;
	int level{ 1 };
	int score{ 0 };
};
