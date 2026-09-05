#include "MenuState.h"

#include <utility>

#include <SFML/Graphics/RenderWindow.hpp>

#include "audio/AudioManager.h"
#include "input/gamepad/GamepadManager.h"
#include "utils/ConfigEnums.h"

MenuState::MenuState(StateStack& stateStack, StateContext context, sf::Color cursorGlow)
	: State(stateStack, context)
	, chrome(context.assets, context.logicalSize, cursorGlow)
{
}

void MenuState::Update(float deltaTime)
{
	chrome.Update(deltaTime);

	if (pendingTransition.has_value() && !chrome.IsFading())
	{
		const std::function<void()> action{ std::move(*pendingTransition) };
		pendingTransition.reset();
		action();
		return;
	}

	OnUpdate(deltaTime);
}

void MenuState::Render()
{
	chrome.DrawBackground(GetContext().window);
	OnRender();
}

void MenuState::RenderOverlay()
{
	chrome.DrawOverlay(GetContext().window, GetContext().gamepad.IsInUse());
}

void MenuState::PlayPressSound()
{
	GetContext().audio.PlaySound(
		Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f, SoundPlayback::StopPrevious);
}

void MenuState::BeginTransition(float fadeOutSeconds, std::function<void()> onFadeComplete)
{
	if (pendingTransition.has_value())
		return;

	pendingTransition = std::move(onFadeComplete);
	chrome.StartFadeOut(fadeOutSeconds);
}

bool MenuState::IsTransitioning() const noexcept
{
	return pendingTransition.has_value();
}

void MenuState::ResetTransition() noexcept
{
	pendingTransition.reset();
}

void MenuState::OnUpdate(float)
{
}
