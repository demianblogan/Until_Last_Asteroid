#include "Health.h"

#include <algorithm>

Health::Health(int maximum) noexcept
{
    SetMaximum(maximum);
}

void Health::SetMaximum(int newMaximum, bool restore) noexcept
{
    maximum = std::max(1, newMaximum);
    current = restore ? maximum : std::min(current, maximum);
}

bool Health::ApplyDamage(int amount) noexcept
{
    if (amount <= 0 || IsDepleted())
        return false;

    current = std::max(0, current - amount);
    return true;
}

bool Health::Restore(int amount) noexcept
{
    if (amount <= 0 || current >= maximum)
        return false;

    current = std::min(maximum, current + amount);
    return true;
}

void Health::Reset() noexcept
{
    current = maximum;
}

int Health::GetCurrent() const noexcept
{
    return current;
}

int Health::GetMaximum() const noexcept
{
    return maximum;
}

float Health::GetRatio() const noexcept
{
    return static_cast<float>(current) / static_cast<float>(maximum);
}

bool Health::IsDepleted() const noexcept
{
    return current <= 0;
}
