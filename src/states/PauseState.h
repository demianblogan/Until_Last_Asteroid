#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "states/State.h"
#include "ui/MenuButton.h"
#include "ui/NeonGlow.h"

namespace sf { class Shader; }

class PauseState final : public State
{
public:
    PauseState(StateStack& stateStack, StateContext context);
    ~PauseState() override;

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;
    [[nodiscard]] bool IsTransparent() const noexcept override;

private:
    void CaptureBlurredFrame();
    void SelectPrevious();
    void SelectNext();
    void Select(std::size_t index, bool playSound = true);
    void UpdateMouseSelection(sf::Vector2i pixelPosition);
    void BeginActivation(std::size_t index);
    void CompleteActivation(std::size_t index);

    sf::Texture windowSnapshot;
    sf::RenderTexture horizontalBlur;
    sf::RenderTexture blurredFrame;
    sf::Shader& blurShader;
    sf::RectangleShape darkOverlay;
    sf::Text titleGlow;
    sf::Text title;
    NeonGlow neonGlow;
    std::vector<MenuButton> buttons;
    std::size_t selectedIndex{ 0 };
    std::size_t pendingActivation{ 0 };
    float activationDelayRemaining{ 0.f };
    bool frameCaptured{ false };
    bool activationPending{ false };
    bool musicWasPlaying{ false };
    bool returningToMainMenu{ false };
};
