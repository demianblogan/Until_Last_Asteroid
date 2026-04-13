#include "GameState.h"

int GameState::GetLives() const noexcept
{
	return lives;
}

int GameState::GetLevel() const noexcept
{
	return level;
}

int GameState::GetScore() const noexcept
{
	return score;
}

bool GameState::IsStart() const noexcept
{
	return state == State::Start;
}

bool GameState::IsPlaying() const noexcept
{
	return state == State::Playing;
}

bool GameState::IsGameOver() const noexcept
{
	return state == State::GameOver;
}

bool GameState::IsLevelComplete() const noexcept
{
	return state == State::LevelComplete;
}

bool GameState::IsWin() const noexcept
{
	return state == State::Win;
}

void GameState::SetWin() noexcept
{
	state = State::Win;
}

void GameState::SetLevelComplete() noexcept
{
	state = State::LevelComplete;
}

void GameState::StartGame() noexcept
{
	state = State::Playing;
}

void GameState::SetGameOver() noexcept
{
	state = State::GameOver;
}

void GameState::Reset() noexcept
{
	lives = 3;
	level = 1;
	score = 0;
	state = State::Start;
}

void GameState::AddScore(int points) noexcept
{
	score += points * level;
}

void GameState::LoseLife() noexcept
{
	// Does not change state to GameOver automatically
	if (lives > 0)
		lives--;
}

void GameState::NextLevel() noexcept
{
	level++;
}