#include "GameplaySession.h"

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

void GameplaySession::ConfigurePlayerHealth(int maximumHealth) noexcept
{
	playerHealth.SetMaximum(maximumHealth);
}

void GameplaySession::Reset() noexcept
{
	playerHealth.Reset();
	level = 1;
	score = 0;
	state = State::Playing;
}

void GameplaySession::AddScore(int points) noexcept
{
	score += points;
}

void GameplaySession::NextLevel() noexcept
{
	level++;
	state = State::Playing;
}
