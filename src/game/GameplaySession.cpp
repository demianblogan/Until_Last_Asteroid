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

const Shield& GameplaySession::GetPlayerShield() const noexcept
{
	return playerShield;
}

void GameplaySession::ConfigurePlayerHealth(int maximumHealth) noexcept
{
	playerHealth.SetMaximum(maximumHealth);
}

void GameplaySession::ConfigureShield(float capacity, float duration) noexcept
{
	playerShield.Configure(capacity, duration);
}

void GameplaySession::Update(float deltaTime) noexcept
{
	playerShield.Update(deltaTime);
}

GameplaySession::PlayerDamageResult GameplaySession::ApplyPlayerDamage(int damage) noexcept
{
	if (damage <= 0)
		return {};

	const float shieldBefore{ playerShield.GetCurrent() };
	const int remainingDamage{ playerShield.AbsorbDamage(damage) };
	const bool shieldDamaged{ playerShield.GetCurrent() < shieldBefore };
	bool healthDamaged{ false };
	if (remainingDamage > 0)
		healthDamaged = playerHealth.ApplyDamage(remainingDamage);
	return { shieldDamaged || healthDamaged, shieldDamaged, healthDamaged };
}

bool GameplaySession::RestorePlayerHealth(int amount) noexcept
{
	return playerHealth.Restore(amount);
}

void GameplaySession::ActivateShield() noexcept
{
	playerShield.Activate();
}

void GameplaySession::Reset() noexcept
{
	playerHealth.Reset();
	playerShield.Deactivate();
	level = 1;
	score = 0;
	levelStartScore = 0;
	state = State::Playing;
}

void GameplaySession::StartAtLevel(int levelNumber, int accumulatedScore) noexcept
{
	playerHealth.Reset();
	playerShield.Deactivate();
	level = levelNumber;
	score = accumulatedScore;
	levelStartScore = accumulatedScore;
	state = State::Playing;
}

void GameplaySession::RestartLevel() noexcept
{
	playerHealth.Reset();
	playerShield.Deactivate();
	score = levelStartScore;
	state = State::Playing;
}

void GameplaySession::AddScore(int points) noexcept
{
	score += points;
}

void GameplaySession::NextLevel() noexcept
{
	playerShield.Deactivate();
	levelStartScore = score;
	level++;
	state = State::Playing;
}

int GameplaySession::GetLevelScore() const noexcept
{
	return score - levelStartScore;
}
