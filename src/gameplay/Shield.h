#pragma once

class Shield
{
public:
    void Configure(float capacity, float duration) noexcept;
    void Activate(float extraDuration = 0.f) noexcept;
    void Deactivate() noexcept;
    void Update(float deltaTime) noexcept;
    [[nodiscard]] int AbsorbDamage(int damage) noexcept;

    [[nodiscard]] bool IsActive() const noexcept;
    [[nodiscard]] float GetCurrent() const noexcept;
    [[nodiscard]] float GetCapacity() const noexcept;
    [[nodiscard]] float GetRatio() const noexcept;
    [[nodiscard]] bool IsHitFlashing() const noexcept;
    [[nodiscard]] float GetHitFlashRatio() const noexcept;

private:
    static constexpr float HitFlashDuration{ 0.36f };

    float current{ 0.f };
    float capacity{ 100.f };
    float duration{ 10.f };
    float activeDuration{ 10.f };
    float hitFlashRemaining{ 0.f };
};
