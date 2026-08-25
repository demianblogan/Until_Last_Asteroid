#pragma once

#include <cstddef>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "gameplay/GameplayLaunch.h"
#include "states/State.h"
#include "ui/GlowingCursor.h"
#include "ui/MenuBackground.h"
#include "ui/MenuButton.h"
#include "rendering/NeonGlow.h"
#include "ui/ScreenFade.h"

class CampaignMenuState final : public State
{
public:
    CampaignMenuState(StateStack& stateStack, StateContext context);

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderOverlay() override;

private:
    enum class DialogMode
    {
        None,
        OverwriteCampaign,
        TutorialChoice
    };

    enum class MenuAction
    {
        ContinueCampaign,
        StartNewCampaign,
        SelectLevel,
        HordeMode,
        RunMode,
        Back
    };

    void SelectPrevious();
    void SelectNext();
    void Select(std::size_t index, bool playSound = true);
    void UpdateMouseSelection(sf::Vector2i pixelPosition);
    void ActivateSelected();
    void OpenOverwriteConfirmation();
    void OpenTutorialChoice();
    void CloseDialog();
    void SelectDialogOption(std::size_t index, bool playSound = true);
    void ActivateDialogOption();
    void StartNewCampaign();
    void ChooseTutorial(bool playTutorial);
    void BeginGameplay(GameplayLaunchMode mode);
    void PlayPressSound();

    MenuBackground background;
    NeonGlow buttonGlow;
    NeonGlow titleGlow;
    NeonGlow dialogGlow;
    GlowingCursor menuCursor;
    ScreenFade screenFade;
    sf::Text title;
    sf::Text statusText;
    sf::RectangleShape dialogShade;
    sf::RectangleShape dialogPanel;
    sf::Text dialogTitle;
    sf::Text dialogMessage;
    std::vector<MenuButton> buttons;
    std::vector<MenuAction> buttonActions;
    std::vector<MenuButton> dialogButtons;
    std::size_t selectedIndex{ 0u };
    std::size_t dialogSelectedIndex{ 1u };
    DialogMode dialogMode{ DialogMode::None };
    bool launchingGameplay{ false };
	bool launchingUpgrades{ false };
	bool launchingLevelSelect{ false };
};
