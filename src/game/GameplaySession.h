#pragma once

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

	[[nodiscard]] int GetLives() const noexcept;
	[[nodiscard]] int GetLevel() const noexcept;
	[[nodiscard]] int GetScore() const noexcept;

	[[nodiscard]] bool IsPlaying() const noexcept;
	[[nodiscard]] bool IsGameOver() const noexcept;
	[[nodiscard]] bool IsLevelComplete() const noexcept;
	[[nodiscard]] bool IsWin() const noexcept;

	void SetWin() noexcept;

	void SetLevelComplete() noexcept;
	void SetGameOver() noexcept;

	void Reset() noexcept;
	void AddScore(int points) noexcept; // Score scales with current level
	void LoseLife() noexcept;
	void NextLevel() noexcept;

private:
	State state{ State::Playing };
	int lives{ 3 };
	int level{ 1 };
	int score{ 0 };
};
