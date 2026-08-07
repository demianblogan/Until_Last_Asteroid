#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

#include "settings/GameSettings.h"
#include "states/State.h"
#include "ui/MenuBackground.h"
#include "ui/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class OptionsState final : public State
{
public:
    OptionsState(StateStack& stateStack, StateContext context);
    ~OptionsState() override;

    void HandleEvent(const sf::Event& event) override;
    void Update(float deltaTime) override;
    void Render() override;

private:
    enum class Page
    {
        Root,
        Graphics,
        Audio,
        Controls
    };

    enum class RowKind
    {
        Button,
        Toggle,
        Slider,
        Choice,
        Dropdown,
        Binding
    };

    enum class Action
    {
        OpenGraphics,
        OpenAudio,
        OpenControls,
        Back,
        ResetAll,
        Resolution,
        WindowMode,
        ShowFps,
        VerticalSync,
        FrameRateLimit,
        ResetGraphics,
        MusicVolume,
        SoundVolume,
        ResetAudio,
        MoveUp,
        MoveDown,
        MoveLeft,
        MoveRight,
        Fire,
        ResetControls
    };

    struct Row
    {
        std::string label;
        RowKind kind;
        Action action;
        bool enabled{ true };
        sf::FloatRect bounds;
    };

    void SetPage(Page newPage);
    void RefreshTitle();
    void RebuildRows();
    void RebuildRowTextCache();
    void RefreshRowTextValues();
    void Select(std::size_t index, bool playSound = true);
    void SelectPrevious();
    void SelectNext();
    void ActivateSelected();
    void AdjustSelected(int direction);
    void HandleMousePosition(sf::Vector2i pixelPosition);
    void HandleMousePress(sf::Vector2i pixelPosition);
    void HandleMouseWheel(float delta);
    void UpdateSliderFromMouse(sf::Vector2f position);

    void OpenDropdown(Action action);
    void CloseDropdown();
    void MoveDropdownSelection(int direction);
    void EnsureDropdownSelectionVisible();
    void HandleDropdownMouseMove(sf::Vector2f position);
    void HandleDropdownMousePress(sf::Vector2f position);
    void UpdateDropdownScrollbar(sf::Vector2f position);
    void ApplyDropdownSelection();

    void Execute(Action action);
    void SaveSettings();
    void SaveAndApplyAudio();
    void SaveAndApplyLiveGraphics();
    void BeginDisplayChange(const GraphicsSettings& previous);
    void ConfirmDisplayChange();
    void RevertDisplayChange();
    void SelectDialogOption(std::size_t index, bool playSound = true);
    void ActivateDialogOption(std::size_t index);
    void ApplyResolution(std::size_t resolutionIndex);
    void ApplyWindowMode(WindowMode mode);
    [[nodiscard]] bool RequiresWindowRecreation(
        const GraphicsSettings& before,
        const GraphicsSettings& after) const noexcept;

    void BeginBinding(Action action);
    void ApplyBinding(ControlBinding binding);
    [[nodiscard]] ControlBinding* GetBinding(Action action);
    [[nodiscard]] const ControlBinding* GetBinding(Action action) const;

    [[nodiscard]] std::string GetRowValue(const Row& row) const;
    [[nodiscard]] std::string GetBindingName(const ControlBinding& binding) const;
    [[nodiscard]] std::size_t FindCurrentResolution() const;
    [[nodiscard]] std::size_t GetDropdownItemCount() const;
    [[nodiscard]] std::string GetDropdownItemLabel(std::size_t index) const;
    [[nodiscard]] sf::FloatRect GetValueBoxBounds(const Row& row) const;
    [[nodiscard]] sf::FloatRect GetDropdownItemBounds(std::size_t visibleIndex) const;
    [[nodiscard]] sf::FloatRect GetDropdownScrollbarBounds() const;
    [[nodiscard]] bool IsSelectedRowEnabled() const;

    void DrawTitle(sf::RenderTarget& target) const;
    void DrawRows(sf::RenderTarget& target);
    void DrawRow(
        sf::RenderTarget& target,
        const Row& row,
        std::size_t index,
        const sf::RenderStates& states);
    void DrawSlider(
        sf::RenderTarget& target,
        const Row& row,
        float value,
        const sf::RenderStates& states) const;
    void DrawToggle(
        sf::RenderTarget& target,
        const Row& row,
        bool value,
        const sf::RenderStates& states);
    void DrawDropdown(sf::RenderTarget& target);
    void DrawDropdownItem(
        sf::RenderTarget& target,
        const sf::FloatRect& bounds,
        std::size_t itemIndex,
        bool selected,
        const sf::RenderStates& states) const;
    void DrawDialog(sf::RenderTarget& target);
    void DrawDialogButton(
        sf::RenderTarget& target,
        const sf::FloatRect& bounds,
        const std::string& label,
        bool selected,
        const sf::RenderStates& states) const;
    void DrawCenteredText(
        sf::RenderTarget& target,
        const std::string& value,
        float centerX,
        float y,
        unsigned int size,
        sf::Color color) const;
    void DrawText(
        sf::RenderTarget& target,
        const std::string& value,
        sf::Vector2f position,
        unsigned int size,
        sf::Color color) const;

    static constexpr float DisplayConfirmationDuration{ 10.f };

    MenuBackground background;
    sf::RectangleShape shade;
    sf::Text titleGlow;
    sf::Text title;
    NeonGlow neonGlow;
    NeonGlow dropdownGlow;
    NeonGlow dialogGlow;
    Page page{ Page::Root };
    std::vector<Row> rows;
    std::vector<sf::Text> rowLabels;
    std::vector<sf::Text> rowValues;
    std::vector<sf::Text> rowHints;
    sf::Text toggleOnText;
    sf::Text toggleOffText;
    std::size_t selectedIndex{ 0u };

    bool dropdownOpen{ false };
    Action dropdownAction{ Action::Resolution };
    std::size_t dropdownIndex{ 0u };
    std::size_t dropdownFirstVisible{ 0u };
    bool dropdownScrollbarDragging{ false };
    bool sliderDragging{ false };
    std::optional<Action> pendingBinding;

    bool displayConfirmationOpen{ false };
    std::size_t dialogSelectedIndex{ 0u };
    float displayConfirmationRemaining{ 0.f };
    GraphicsSettings previousGraphics;
    bool saveFailed{ false };
};
