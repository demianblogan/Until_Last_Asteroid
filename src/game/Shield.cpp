#include "Shield.h"

#include <algorithm>
#include <cmath>

void Shield::Configure(float newCapacity, float newDuration) noexcept
{
    capacity = std::max(1.f, newCapacity);
    duration = std::max(0.1f, newDuration);
    current = 0.f;
    hitFlashRemaining = 0.f;
}

void Shield::Activate() noexcept
{
    current = capacity;
    hitFlashRemaining = 0.f;
}

void Shield::Deactivate() noexcept
{
    current = 0.f;
    hitFlashRemaining = 0.f;
}

void Shield::Update(float deltaTime) noexcept
{
    if (deltaTime <= 0.f)
        return;

    hitFlashRemaining = std::max(0.f, hitFlashRemaining - deltaTime);
    if (IsActive())
        current = std::max(0.f, current - capacity / duration * deltaTime);
}

int Shield::AbsorbDamage(int damage) noexcept
{
    if (damage <= 0 || !IsActive())
        return std::max(0, damage);

    const float absorbed{ std::min(current, static_cast<float>(damage)) };
    current = std::max(0.f, current - absorbed);
    if (absorbed > 0.f)
        hitFlashRemaining = HitFlashDuration;
    return std::max(0, damage - static_cast<int>(std::floor(absorbed + 0.001f)));
}

bool Shield::IsActive() const noexcept
{
    return current > 0.f;
}

float Shield::GetCurrent() const noexcept
{
    return current;
}

float Shield::GetCapacity() const noexcept
{
    return capacity;
}

float Shield::GetRatio() const noexcept
{
    return capacity > 0.f ? current / capacity : 0.f;
}

bool Shield::IsHitFlashing() const noexcept
{
    return hitFlashRemaining > 0.f;
}

float Shield::GetHitFlashRatio() const noexcept
{
    return hitFlashRemaining / HitFlashDuration;
}
