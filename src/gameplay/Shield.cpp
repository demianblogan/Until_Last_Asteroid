#include "Shield.h"

#include <algorithm>
#include <cmath>

void Shield::Configure(float newCapacity, float newDuration) noexcept
{
	capacity = std::max(1.f, newCapacity);
	duration = std::max(0.1f, newDuration);

	activeDuration = duration;
	currentCharge = 0.f;
	hitFlashRemaining = 0.f;
}

void Shield::Activate(float extraDuration) noexcept
{
	activeDuration = duration + std::max(0.f, extraDuration);
	currentCharge = capacity;
	hitFlashRemaining = 0.f;
}

void Shield::Deactivate() noexcept
{
	currentCharge = 0.f;
	hitFlashRemaining = 0.f;
}

void Shield::Update(float deltaTime) noexcept
{
	if (deltaTime <= 0.f)
		return;

	hitFlashRemaining = std::max(0.f, hitFlashRemaining - deltaTime);

	if (IsActive())
		currentCharge = std::max(0.f, currentCharge - capacity / activeDuration * deltaTime);
}

int Shield::AbsorbDamage(int damage) noexcept
{
	if (damage <= 0 || !IsActive())
		return std::max(0, damage);

	const float absorbed = std::min(currentCharge, static_cast<float>(damage));
	currentCharge = std::max(0.f, currentCharge - absorbed);

	if (absorbed > 0.f)
		hitFlashRemaining = HitFlashDuration;

	// `absorbed` is a float but the damage we pass through is an int, so it
	// has to be floored back to a whole number. The +0.001f nudges values
	// that are a hair below an integer (e.g. 4.99999 from float rounding)
	// up to that integer before flooring, so the shield doesn't "leak" one
	// point of damage per hit purely from accumulated float error.
	return std::max(0, damage - static_cast<int>(std::floor(absorbed + 0.001f)));
}

bool Shield::IsActive() const noexcept
{
	return currentCharge > 0.f;
}

float Shield::GetCurrent() const noexcept
{
	return currentCharge;
}

float Shield::GetCapacity() const noexcept
{
	return capacity;
}

float Shield::GetRatio() const noexcept
{
	// No `capacity > 0.f` guard needed: Configure() clamps it to at least
	// 1.f and the default member value is 100.f, so it can never reach
	// zero for the lifetime of a Shield -- same as Health::GetRatio().
	return currentCharge / capacity;
}

bool Shield::IsHitFlashing() const noexcept
{
	return hitFlashRemaining > 0.f;
}

float Shield::GetHitFlashRatio() const noexcept
{
	return hitFlashRemaining / HitFlashDuration;
}