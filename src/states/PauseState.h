#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuButtonList.h"
#include "rendering/NeonGlow.h"
#include "ui/ScreenFade.h"

namespace sf { class Shader; }

class PauseState final : public State
{
public:
	PauseState(StateStack& stateStack, StateContext context);
	~PauseState() override;

	void HandleEvent(const sf::Event& event) override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;
	[[nodiscard]] bool IsTransparent() const noexcept override;

private:
	enum class PauseAction
	{
		Resume,
		RestartLevel,
		SkipTutorial,
		Options,
		MainMenu
	};

	void CaptureBlurredFrame();
	void BeginActivation(std::size_t index);
	void CompleteActivation(std::size_t index);
	void RefreshLocalizedContent();

	sf::Texture windowSnapshot;
	sf::RenderTexture horizontalBlur;
	sf::RenderTexture blurredFrame;
	sf::Shader& blurShader;
	sf::RectangleShape darkOverlay;
	sf::Text titleGlow;
	sf::Text title;
	Rendering::NeonGlow neonGlow;
	UI::GlowingCursor menuCursor;
	UI::ScreenFade screenFade;
	UI::MenuButtonList buttonList;
	std::vector<PauseAction> buttonActions;
	sf::Vector2u capturedWindowSize{};
	std::size_t pendingActivation{ 0 };
	std::size_t localizationRevision{ 0u };
	float activationDelayRemaining{ 0.f };
	bool isFrameCaptured{ false };
	bool isActivationPending{ false };
	bool wasMusicPlaying{ false };
	bool isReturningToMainMenu{ false };
};
