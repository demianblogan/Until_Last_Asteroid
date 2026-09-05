#pragma once

#include <functional>
#include <optional>

#include <SFML/Graphics/Color.hpp>

#include "states/State.h"
#include "ui/MenuChrome.h"
#include "ui/MenuTheme.h"

// Common base for the front-end screens (main menu, campaign menu, credits,
// achievements, ...). It owns the shared MenuChrome (background + cursor +
// fade), the "play the click sound" helper, and the fade-out-then-navigate
// pattern every screen repeats, so a concrete screen is left with just its
// own content and input handling.
//
// Not everything front-end derives from this: PauseState draws a blurred
// snapshot instead of the parallax background, ShipUpgradesState has its own
// backdrop and 2D navigation, and CompanySplashState has no chrome at all --
// those stay on State directly.
class MenuState : public State
{
public:
	void Update(float deltaTime) final;
	void Render() final;
	void RenderOverlay() override;

protected:
	MenuState(StateStack& stateStack, StateContext context,
		sf::Color cursorGlow = UI::MenuTheme::InterfaceGlow);

	[[nodiscard]] UI::MenuChrome& Chrome() noexcept { return chrome; }

	void PlayPressSound();

	// Fade the screen out over `fadeOutSeconds`; once the fade finishes, run
	// `onFadeComplete` (a RequestPop / RequestClear + RequestPush / ...).
	// Until then IsTransitioning() is true, which a screen uses to swallow
	// input.
	void BeginTransition(float fadeOutSeconds, std::function<void()> onFadeComplete);
	[[nodiscard]] bool IsTransitioning() const noexcept;

	// Drop a not-yet-fired transition -- for a cached screen that is being
	// reactivated after having started (but not finished) an exit.
	void ResetTransition() noexcept;

	// Screens implement these instead of Update() / Render(): OnUpdate runs
	// every frame after the chrome has ticked and any finished transition has
	// fired; OnRender runs after the chrome background is drawn.
	virtual void OnUpdate(float deltaTime);
	virtual void OnRender() = 0;

private:
	UI::MenuChrome chrome;
	std::optional<std::function<void()>> pendingTransition;
};
