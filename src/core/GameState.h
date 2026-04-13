#pragma once

class GameState
{
public:
	enum class State
	{
		Start,
		Playing,
		LevelComplete,
		GameOver,
		Win
	};

	[[nodiscard]] int GetLives() const noexcept;
	[[nodiscard]] int GetLevel() const noexcept;
	[[nodiscard]] int GetScore() const noexcept;

	[[nodiscard]] bool IsStart() const noexcept;
	[[nodiscard]] bool IsPlaying() const noexcept;
	[[nodiscard]] bool IsGameOver() const noexcept;
	[[nodiscard]] bool IsLevelComplete() const noexcept;
	[[nodiscard]] bool IsWin() const noexcept;

	void SetWin() noexcept;

	void SetLevelComplete() noexcept;
	void StartGame() noexcept;
	void SetGameOver() noexcept;

	void Reset() noexcept;
	void AddScore(int points) noexcept; // Score scales with current level
	void LoseLife() noexcept;
	void NextLevel() noexcept;

private:
	State state{ State::Start };
	int lives{ 3 };
	int level{ 1 };
	int score{ 0 };
};