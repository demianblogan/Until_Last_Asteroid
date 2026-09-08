#pragma once

class Health
{
public:
	explicit Health(int maximum = 1) noexcept;

	void SetMaximum(int maximum, bool needToRestoreToFull = true) noexcept;
	[[nodiscard]] bool ApplyDamage(int amount) noexcept;
	[[nodiscard]] bool Restore(int amount) noexcept;
	void Reset() noexcept;

	[[nodiscard]] int GetCurrent() const noexcept;
	[[nodiscard]] int GetMaximum() const noexcept;
	[[nodiscard]] float GetRatio() const noexcept;
	[[nodiscard]] bool IsDepleted() const noexcept;

private:
	int current = 1;
	int maximum = 1;
};
