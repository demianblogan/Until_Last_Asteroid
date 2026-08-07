#include "GameplaySession.h"

int GameplaySession::GetLives() const noexcept
{
	return lives;
}

int GameplaySession::GetLevel() const noexcept
{
	return level;
}

int GameplaySession::GetScore() const noexcept
{
	return score;
}

bool GameplaySession::IsPlaying() const noexcept
{
	return state == State::Playing;
}

bool GameplaySession::IsGameOver() const noexcept
{
	return state == State::GameOver;
}

bool GameplaySession::IsLevelComplete() const noexcept
{
	return state == State::LevelComplete;
}

bool GameplaySession::IsWin() const noexcept
{
	return state == State::Win;
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

void GameplaySession::Reset() noexcept
{
	lives = 3;
	level = 1;
	score = 0;
	state = State::Playing;
}

void GameplaySession::AddScore(int points) noexcept
{
	score += points * level;
}

void GameplaySession::LoseLife() noexcept
{
	// Does not change state to GameOver automatically
	if (lives > 0)
		lives--;
}

void GameplaySession::NextLevel() noexcept
{
	level++;
	state = State::Playing;
}
