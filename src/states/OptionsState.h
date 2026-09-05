#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Text.hpp>

#include "settings/GameSettings.h"
#include "states/MenuState.h"
#include "rendering/NeonGlow.h"
#include "ui/RoundedRectangleShape.h"

class OptionsState final : public MenuState
{
public:
	enum class Origin
	{
		MainMenu,
		PauseMenu
	};

	OptionsState(
		StateStack& stateStack,
		StateContext context,
		Origin origin = Origin::MainMenu);
	~OptionsState() override;

	void HandleEvent(const sf::Event& event) override;
	void OnUpdate(float deltaTime) override;
	void OnRender() override;
	void RenderOverlay() override;

private:
	enum class Page
	{
		Root,
		Graphics,
		Audio,
		Gameplay,
		Controls,
		Language,
		KeyboardControls,
		GamepadControls
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
		OpenGameplay,
		OpenControls,
		OpenLanguage,
		OpenKeyboardControls,
		OpenGamepadControls,
		Back,
		ResetAll,
		Resolution,
		WindowMode,
		ShowFps,
		VerticalSync,
		FrameRateLimit,
		PostEffects,
		ResetGraphics,
		MusicVolume,
		SoundVolume,
		ResetAudio,
		ScreenShake,
		ShowScorePopups,
		GamepadVibration,
		GamepadAdaptiveTriggers,
		GamepadLightbar,
		ResetGameplay,
		MoveUp,
		MoveDown,
		MoveLeft,
		MoveRight,
		Fire,
		ResetControls,
		SetEnglish,
		SetSpanish,
		SetRussian,
		SetUkrainian,
		SetArabic
	};

	struct Row
	{
		sf::String label;
		RowKind kind;
		Action action;
		bool isEnabled{ true };
		sf::FloatRect bounds;
	};

	void ApplyPage(Page newPage);
	void BeginPageTransition(Page newPage);
	void BeginExit();
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

	[[nodiscard]] sf::String GetRowValue(const Row& row) const;
	[[nodiscard]] sf::String GetBindingName(const ControlBinding& binding) const;
	[[nodiscard]] std::size_t FindCurrentResolution() const;
	[[nodiscard]] std::size_t GetDropdownItemCount() const;
	[[nodiscard]] sf::String GetDropdownItemLabel(std::size_t index) const;
	[[nodiscard]] sf::FloatRect GetDropdownItemBounds(std::size_t visibleIndex) const;
	[[nodiscard]] sf::FloatRect GetDropdownScrollbarBounds() const;
	[[nodiscard]] bool IsSelectedRowEnabled() const;

	void DrawTitle(sf::RenderTarget& target);
	void DrawGamepadLayouts(sf::RenderTarget& target);
	void DrawGamepadLayoutsContent(sf::RenderTarget& target);
	void DrawRows(sf::RenderTarget& target);
	void DrawRow(
		sf::RenderTarget& target,
		const Row& row,
		std::size_t index,
		const sf::RenderStates& states);
	void DrawDropdown(sf::RenderTarget& target);
	void DrawDialog(sf::RenderTarget& target);

	static constexpr float DisplayConfirmationDuration{ 10.f };

	sf::RectangleShape shade;
	sf::Text title;
	Rendering::NeonGlow titleGlow;
	Rendering::NeonGlow neonGlow;
	Rendering::NeonGlow dialogGlow;
	sf::RenderTexture gamepadLayoutCache;
	bool isGamepadLayoutCacheDirty{ true };
	Page page{ Page::Root };
	std::optional<Page> pendingPage;
	std::vector<Row> rows;
	std::vector<sf::Text> rowLabels;
	std::vector<sf::Text> rowValues;
	std::vector<sf::Text> rowHints;
	sf::Text toggleOnText;
	sf::Text toggleOffText;
	std::size_t selectedIndex{ 0u };

	bool isDropdownOpen{ false };
	Action dropdownAction{ Action::Resolution };
	std::size_t dropdownIndex{ 0u };
	std::size_t dropdownFirstVisible{ 0u };
	std::vector<sf::Text> dropdownLabels;
	bool isDropdownScrollbarDragging{ false };
	bool isSliderDragging{ false };
	std::optional<Action> pendingBinding;

	bool isDisplayConfirmationOpen{ false };
	std::size_t dialogSelectedIndex{ 0u };
	float displayConfirmationRemaining{ 0.f };
	GraphicsSettings previousGraphics;
	bool hasSaveFailed{ false };
	Origin origin;
};
