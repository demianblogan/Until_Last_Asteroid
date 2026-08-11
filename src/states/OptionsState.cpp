#include "OptionsState.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "app/DisplayManager.h"
#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "settings/SettingsManager.h"
#include "systems/GamepadManager.h"

namespace
{
    constexpr sf::Color Cyan{ 105, 225, 242 };
    constexpr sf::Color BrightCyan{ 205, 250, 255 };
    constexpr sf::Color Orange{ 255, 190, 72 };
    constexpr sf::Color Muted{ 128, 148, 164 };
    constexpr sf::Color Disabled{ 76, 88, 101 };
    constexpr sf::Color Red{ 245, 92, 92 };
    constexpr sf::Color SelectionGlowColor{ 255, 178, 42 };
    constexpr sf::Color InterfaceGlowColor{ 25, 220, 255 };
    constexpr float StateFadeDuration{ 0.24f };
    constexpr float PageFadeOutDuration{ 0.1f };
    constexpr float PageFadeInDuration{ 0.14f };
    constexpr sf::Vector2f RowPosition{ 260.f, 220.f };
    constexpr sf::Vector2f RowSize{ 1400.f, 82.f };
    constexpr float RowSpacing{ 98.f };
    constexpr float SliderLeft{ 1160.f };
    constexpr float SliderWidth{ 380.f };
    constexpr float ValueBoxLeft{ 1080.f };
    constexpr float ValueBoxWidth{ 500.f };
    constexpr float ValueBoxHeight{ 58.f };
    constexpr float DropdownItemHeight{ 56.f };
    constexpr std::size_t MaximumVisibleDropdownItems{ 6u };
    constexpr std::array<unsigned int, 7> FrameLimits{ 0u, 30u, 60u, 120u, 144u, 240u, 360u };
    const sf::FloatRect DialogConfirmBounds({ 690.f, 580.f }, { 250.f, 58.f });
    const sf::FloatRect DialogCancelBounds({ 980.f, 580.f }, { 250.f, 58.f });
    const sf::FloatRect BindingCancelBounds({ 835.f, 570.f }, { 250.f, 58.f });

    std::string WindowModeName(WindowMode mode)
    {
        switch (mode)
        {
        case WindowMode::Fullscreen:
            return "Fullscreen";
        case WindowMode::Windowed:
            return "Windowed";
        case WindowMode::Borderless:
            return "Borderless";
        }
        return "Unknown";
    }

    std::string MouseButtonName(sf::Mouse::Button button)
    {
        switch (button)
        {
        case sf::Mouse::Button::Left:
            return "Left Mouse";
        case sf::Mouse::Button::Right:
            return "Right Mouse";
        case sf::Mouse::Button::Middle:
            return "Middle Mouse";
        case sf::Mouse::Button::Extra1:
            return "Mouse 4";
        case sf::Mouse::Button::Extra2:
            return "Mouse 5";
        }
        return "Mouse";
    }
}

OptionsState::OptionsState(StateStack& stateStack, StateContext context, Origin optionsOrigin)
    : State(stateStack, context)
    , background(context.assets, context.logicalSize)
    , shade(context.logicalSize)
    , titleGlow(context.assets.Fonts().Get(Config::Font::MenuSemibold), "OPTIONS", 76)
    , title(context.assets.Fonts().Get(Config::Font::MenuSemibold), "OPTIONS", 76)
    , neonGlow(context.assets)
    , dropdownGlow(context.assets)
    , dialogGlow(context.assets)
    , xboxHeadingGlow(context.assets)
    , playStationHeadingGlow(context.assets)
    , menuCursor(
        context.assets,
        Config::Texture::MenuPointer,
        { 6.f, 2.f },
        InterfaceGlowColor)
    , screenFade(context.logicalSize)
    , toggleOnText(context.assets.Fonts().Get(Config::Font::MenuRegular), "ON", 23)
    , toggleOffText(context.assets.Fonts().Get(Config::Font::MenuRegular), "OFF", 23)
    , previousGraphics(context.settings.Get().graphics)
    , origin(optionsOrigin)
{
    context.window.setMouseCursorVisible(false);
    shade.setFillColor(sf::Color(0, 4, 10, 150));

    titleGlow.setFillColor(sf::Color(80, 215, 245, 25));
    titleGlow.setOutlineColor(sf::Color(45, 205, 245, 85));
    titleGlow.setOutlineThickness(8.f);
    title.setFillColor(BrightCyan);
    title.setOutlineColor(sf::Color(2, 14, 25, 235));
    title.setOutlineThickness(3.f);
    ApplyPage(Page::Root);
    screenFade.StartFadeIn(StateFadeDuration);
}

void OptionsState::HandleEvent(const sf::Event& event)
{
    if (screenFade.IsActive())
        return;

    const GamepadManager::NavigationAction navigation{
        GetContext().gamepad.GetNavigationAction(event) };

    if (displayConfirmationOpen)
    {
        using enum GamepadManager::NavigationAction;
        switch (navigation)
        {
        case Left: SelectDialogOption(0u); return;
        case Right: SelectDialogOption(1u); return;
        case Confirm: ActivateDialogOption(dialogSelectedIndex); return;
        case Back: ActivateDialogOption(1u); return;
        default: break;
        }

        if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
        {
            const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
            background.SetMousePosition(point);
            if (DialogConfirmBounds.contains(point))
                SelectDialogOption(0u);
            else if (DialogCancelBounds.contains(point))
                SelectDialogOption(1u);
            return;
        }

        if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
        {
            if (mouse->button == sf::Mouse::Button::Left)
            {
                const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mouse->position) };
                if (DialogConfirmBounds.contains(point))
                    ActivateDialogOption(0u);
                else if (DialogCancelBounds.contains(point))
                    ActivateDialogOption(1u);
            }
            return;
        }

        if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
        {
            if (key->code == sf::Keyboard::Key::Left || key->code == sf::Keyboard::Key::A)
                SelectDialogOption(0u);
            else if (key->code == sf::Keyboard::Key::Right || key->code == sf::Keyboard::Key::D)
                SelectDialogOption(1u);
            else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
                ActivateDialogOption(dialogSelectedIndex);
            else if (key->code == sf::Keyboard::Key::Escape)
                ActivateDialogOption(1u);
        }
        return;
    }

    if (pendingBinding.has_value())
    {
        if (navigation == GamepadManager::NavigationAction::Back)
        {
            GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI);
            pendingBinding.reset();
            return;
        }

        if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
        {
            background.SetMousePosition(GetContext().window.mapPixelToCoords(moved->position));
            return;
        }

        if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
        {
            if (key->code == sf::Keyboard::Key::Escape)
                pendingBinding.reset();
            else if (key->code != sf::Keyboard::Key::Unknown)
                ApplyBinding({ InputDevice::Keyboard, static_cast<int>(key->code) });
        }
        else if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
        {
            const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mouse->position) };
            if (mouse->button == sf::Mouse::Button::Left && BindingCancelBounds.contains(point))
            {
                GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI);
                pendingBinding.reset();
            }
            else
            {
                ApplyBinding({ InputDevice::Mouse, static_cast<int>(mouse->button) });
            }
        }
        return;
    }

    if (dropdownOpen)
    {
        using enum GamepadManager::NavigationAction;
        switch (navigation)
        {
        case Up: MoveDropdownSelection(-1); return;
        case Down: MoveDropdownSelection(1); return;
        case Confirm: ApplyDropdownSelection(); return;
        case Back: CloseDropdown(); return;
        default: break;
        }

        if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
        {
            if (key->code == sf::Keyboard::Key::Up)
                MoveDropdownSelection(-1);
            else if (key->code == sf::Keyboard::Key::Down)
                MoveDropdownSelection(1);
            else if (key->code == sf::Keyboard::Key::Enter)
                ApplyDropdownSelection();
            else if (key->code == sf::Keyboard::Key::Escape)
                CloseDropdown();
        }
        else if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
        {
            const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
            background.SetMousePosition(point);
            if (dropdownScrollbarDragging)
                UpdateDropdownScrollbar(point);
            else
                HandleDropdownMouseMove(point);
        }
        else if (const auto* wheel{ event.getIf<sf::Event::MouseWheelScrolled>() })
        {
            HandleMouseWheel(wheel->delta);
        }
        else if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
        {
            if (mouse->button == sf::Mouse::Button::Left)
                HandleDropdownMousePress(
                    GetContext().window.mapPixelToCoords(mouse->position));
        }
        else if (event.is<sf::Event::MouseButtonReleased>())
        {
            dropdownScrollbarDragging = false;
        }
        return;
    }

    if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
    {
        const sf::Vector2f point{ GetContext().window.mapPixelToCoords(moved->position) };
        background.SetMousePosition(point);
        if (sliderDragging)
            UpdateSliderFromMouse(point);
        else
            HandleMousePosition(moved->position);
        return;
    }

    if (event.is<sf::Event::MouseButtonReleased>())
    {
        sliderDragging = false;
        return;
    }

    if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
    {
        if (mouse->button == sf::Mouse::Button::Left)
            HandleMousePress(mouse->position);
        return;
    }

    using enum GamepadManager::NavigationAction;
    switch (navigation)
    {
    case Up: SelectPrevious(); return;
    case Down: SelectNext(); return;
    case Left: AdjustSelected(-1); return;
    case Right: AdjustSelected(1); return;
    case Confirm: ActivateSelected(); return;
    case Back: Execute(Action::Back); return;
    default: break;
    }

    if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
    {
        switch (key->code)
        {
        case sf::Keyboard::Key::Up:
        case sf::Keyboard::Key::W:
            SelectPrevious();
            break;
        case sf::Keyboard::Key::Down:
        case sf::Keyboard::Key::S:
            SelectNext();
            break;
        case sf::Keyboard::Key::Left:
        case sf::Keyboard::Key::A:
            AdjustSelected(-1);
            break;
        case sf::Keyboard::Key::Right:
        case sf::Keyboard::Key::D:
            AdjustSelected(1);
            break;
        case sf::Keyboard::Key::Enter:
        case sf::Keyboard::Key::Space:
            ActivateSelected();
            break;
        case sf::Keyboard::Key::Escape:
        case sf::Keyboard::Key::Backspace:
            Execute(Action::Back);
            break;
        default:
            break;
        }
    }
}

void OptionsState::Update(float deltaTime)
{
    background.Update(deltaTime);
    neonGlow.Update(deltaTime);
    dropdownGlow.Update(deltaTime);
    dialogGlow.Update(deltaTime);
    xboxHeadingGlow.Update(deltaTime);
    playStationHeadingGlow.Update(deltaTime);
    menuCursor.Update(deltaTime);

    screenFade.Update(deltaTime);
    if (!screenFade.IsActive())
    {
        if (exitPending)
        {
            exitPending = false;
            RequestPop();
            return;
        }
        if (pendingPage.has_value())
        {
            const Page nextPage{ *pendingPage };
            pendingPage.reset();
            ApplyPage(nextPage);
            screenFade.StartFadeIn(PageFadeInDuration);
        }
    }

    if (!displayConfirmationOpen)
        return;

    displayConfirmationRemaining -= deltaTime;
    if (displayConfirmationRemaining <= 0.f)
        RevertDisplayChange();
}

void OptionsState::Render()
{
    sf::RenderWindow& window{ GetContext().window };
    background.Draw(window);
    window.draw(shade);
    DrawTitle(window);
    if (page == Page::GamepadControls)
        DrawGamepadLayouts(window);
    DrawRows(window);
    if (dropdownOpen)
        DrawDropdown(window);
    if (pendingBinding.has_value() || displayConfirmationOpen)
        DrawDialog(window);
}

void OptionsState::RenderOverlay()
{
    if (!screenFade.IsActive() && !GetContext().gamepad.IsUsingGamepad())
        menuCursor.Draw(GetContext().window);
    screenFade.Draw(GetContext().window);
}

void OptionsState::ApplyPage(Page newPage)
{
    page = newPage;
    RefreshTitle();
    selectedIndex = 0u;
    dropdownOpen = false;
    pendingBinding.reset();
    RebuildRows();
}

void OptionsState::BeginPageTransition(Page newPage)
{
    if (newPage == page || pendingPage.has_value() || exitPending)
        return;
    pendingPage = newPage;
    screenFade.StartFadeOut(PageFadeOutDuration);
}

void OptionsState::BeginExit()
{
    if (exitPending || pendingPage.has_value())
        return;
    exitPending = true;
    screenFade.StartFadeOut(StateFadeDuration);
}

OptionsState::~OptionsState()
{
    if (GetContext().window.isOpen())
        GetContext().window.setMouseCursorVisible(false);
}

void OptionsState::RefreshTitle()
{
    std::string value;
    switch (page)
    {
    case Page::Root: value = "OPTIONS"; break;
    case Page::Graphics: value = "GRAPHICS"; break;
    case Page::Audio: value = "AUDIO"; break;
    case Page::Gameplay: value = "GAMEPLAY"; break;
    case Page::Controls: value = "CONTROLS"; break;
    case Page::KeyboardControls: value = "KEYBOARD"; break;
    case Page::GamepadControls: value = "GAMEPAD"; break;
    }

    const auto center{ [this, &value](sf::Text& text)
        {
            text.setString(value);
            const sf::FloatRect bounds{ text.getLocalBounds() };
            text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
                bounds.position.y + bounds.size.y * 0.5f });
            text.setPosition({ GetContext().logicalSize.x * 0.5f, 105.f });
        } };
    center(titleGlow);
    center(title);
}

void OptionsState::RebuildRows()
{
    rows.clear();
    const auto add{ [this](std::string label, RowKind kind, Action action, bool enabled = true)
        {
            const float y{ RowPosition.y + RowSpacing * static_cast<float>(rows.size()) };
            rows.push_back({ std::move(label), kind, action, enabled,
                sf::FloatRect({ RowPosition.x, y }, RowSize) });
        } };

    switch (page)
    {
    case Page::Root:
        add("Graphics", RowKind::Button, Action::OpenGraphics);
        add("Audio", RowKind::Button, Action::OpenAudio);
        add("Gameplay", RowKind::Button, Action::OpenGameplay);
        add("Controls", RowKind::Button, Action::OpenControls);
        add("Restore Defaults", RowKind::Button, Action::ResetAll);
        add(origin == Origin::PauseMenu ? "Back to Pause Menu" : "Back to Main Menu",
            RowKind::Button, Action::Back);
        break;
    case Page::Graphics:
        add("Display Resolution", RowKind::Dropdown, Action::Resolution,
            GetContext().settings.Get().graphics.windowMode != WindowMode::Borderless);
        add("Window Mode", RowKind::Dropdown, Action::WindowMode);
        add("Show FPS", RowKind::Toggle, Action::ShowFps);
        add("Vertical Synchronization", RowKind::Toggle, Action::VerticalSync);
        add("Frame Rate Limit", RowKind::Choice, Action::FrameRateLimit,
            !GetContext().settings.Get().graphics.verticalSync);
        add("Post Effects", RowKind::Toggle, Action::PostEffects);
        add("Restore Graphics Defaults", RowKind::Button, Action::ResetGraphics);
        add("Back", RowKind::Button, Action::Back);
        break;
    case Page::Gameplay:
        add("Screen Shake", RowKind::Toggle, Action::ScreenShake);
        add("Show Score Popups", RowKind::Toggle, Action::ShowScorePopups);
        add("Restore Gameplay Defaults", RowKind::Button, Action::ResetGameplay);
        add("Back", RowKind::Button, Action::Back);
        break;
    case Page::Audio:
        add("Music", RowKind::Slider, Action::MusicVolume);
        add("Sounds", RowKind::Slider, Action::SoundVolume);
        add("Restore Audio Defaults", RowKind::Button, Action::ResetAudio);
        add("Back", RowKind::Button, Action::Back);
        break;
    case Page::Controls:
        add("Keyboard", RowKind::Button, Action::OpenKeyboardControls);
        add("Gamepad", RowKind::Button, Action::OpenGamepadControls);
        add("Back", RowKind::Button, Action::Back);
        break;
    case Page::KeyboardControls:
        add("Move Up", RowKind::Binding, Action::MoveUp);
        add("Move Down", RowKind::Binding, Action::MoveDown);
        add("Move Left", RowKind::Binding, Action::MoveLeft);
        add("Move Right", RowKind::Binding, Action::MoveRight);
        add("Fire", RowKind::Binding, Action::Fire);
        add("Restore Controls Defaults", RowKind::Button, Action::ResetControls);
        add("Back", RowKind::Button, Action::Back);
        break;
    case Page::GamepadControls:
        add("Return to Controls Options", RowKind::Button, Action::Back);
        rows.back().bounds.position.x = 580.f;
        rows.back().bounds.position.y = 880.f;
        rows.back().bounds.size.x = 760.f;
        break;
    }

    if (!rows.empty() && !rows[selectedIndex].enabled)
        SelectNext();

    RebuildRowTextCache();
    neonGlow.Invalidate();
}

void OptionsState::RebuildRowTextCache()
{
    const sf::Font& font{ GetContext().assets.Fonts().Get(Config::Font::MenuRegular) };
    rowLabels.clear();
    rowValues.clear();
    rowHints.clear();
    rowLabels.reserve(rows.size());
    rowValues.reserve(rows.size());
    rowHints.reserve(rows.size());

    for (const Row& row : rows)
    {
        rowLabels.emplace_back(font, row.label, 30);
        rowLabels.back().setPosition(row.bounds.position + sf::Vector2f{ 34.f, 20.f });

        rowValues.emplace_back(font, GetRowValue(row), 28);
        rowHints.emplace_back(font, "", 14);
        if (row.kind == RowKind::Dropdown)
        {
            const sf::FloatRect bounds{ GetValueBoxBounds(row) };
            rowValues.back().setCharacterSize(row.enabled ? 25u : 21u);
            rowValues.back().setPosition(
                bounds.position + sf::Vector2f{ 20.f, row.enabled ? 13.f : 5.f });
            if (!row.enabled)
            {
                rowHints.back().setString("Controlled by desktop in Borderless mode");
                rowHints.back().setPosition(bounds.position + sf::Vector2f{ 20.f, 33.f });
                rowHints.back().setFillColor(Red);
            }
        }
        else if (row.kind == RowKind::Slider)
        {
            rowValues.back().setCharacterSize(24u);
            rowValues.back().setPosition({ 1565.f, row.bounds.position.y + 23.f });
        }
        else
        {
            rowValues.back().setPosition({ 1160.f, row.bounds.position.y + 20.f });
        }

        if (page == Page::GamepadControls && row.action == Action::Back)
        {
            const sf::FloatRect labelBounds{ rowLabels.back().getLocalBounds() };
            rowLabels.back().setOrigin({
                labelBounds.position.x + labelBounds.size.x * 0.5f,
                labelBounds.position.y + labelBounds.size.y * 0.5f });
            rowLabels.back().setPosition({
                row.bounds.position.x + row.bounds.size.x * 0.5f,
                row.bounds.position.y + row.bounds.size.y * 0.5f });
        }
    }
}

void OptionsState::RefreshRowTextValues()
{
    if (rowValues.size() != rows.size())
    {
        RebuildRowTextCache();
        return;
    }

    for (std::size_t index{ 0u }; index < rows.size(); ++index)
        rowValues[index].setString(GetRowValue(rows[index]));

    neonGlow.Invalidate();
}

void OptionsState::Select(std::size_t index, bool playSound)
{
    if (index >= rows.size() || !rows[index].enabled)
        return;
    const bool changed{ selectedIndex != index };
    selectedIndex = index;
    if (changed)
        neonGlow.Invalidate();
    if (changed && playSound)
        GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f,
            SoundPlayback::Restart);
}

void OptionsState::SelectPrevious()
{
    if (rows.empty())
        return;
    std::size_t index{ selectedIndex };
    do
    {
        index = index == 0u ? rows.size() - 1u : index - 1u;
    } while (!rows[index].enabled && index != selectedIndex);
    Select(index);
}

void OptionsState::SelectNext()
{
    if (rows.empty())
        return;
    std::size_t index{ selectedIndex };
    do
    {
        index = (index + 1u) % rows.size();
    } while (!rows[index].enabled && index != selectedIndex);
    Select(index);
}

void OptionsState::ActivateSelected()
{
    if (!IsSelectedRowEnabled())
        return;
    GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f,
        SoundPlayback::Restart);

    const Row& row{ rows[selectedIndex] };
    if (row.kind == RowKind::Toggle || row.kind == RowKind::Choice)
        AdjustSelected(1);
    else if (row.kind == RowKind::Dropdown)
        OpenDropdown(row.action);
    else if (row.kind == RowKind::Binding)
        BeginBinding(row.action);
    else if (row.kind == RowKind::Button)
        Execute(row.action);
}

void OptionsState::AdjustSelected(int direction)
{
    if (!IsSelectedRowEnabled())
        return;

    const Action action{ rows[selectedIndex].action };
    GameSettings& settings{ GetContext().settings.Edit() };
    if (action == Action::MusicVolume || action == Action::SoundVolume)
    {
        float& value{ action == Action::MusicVolume
            ? settings.audio.musicVolume
            : settings.audio.soundVolume };
        value = std::clamp(value + 5.f * static_cast<float>(direction), 0.f, 100.f);
        SaveAndApplyAudio();
    }
    else if (action == Action::ShowFps)
    {
        settings.graphics.showFps = !settings.graphics.showFps;
        SaveAndApplyLiveGraphics();
    }
    else if (action == Action::VerticalSync)
    {
        settings.graphics.verticalSync = !settings.graphics.verticalSync;
        SaveAndApplyLiveGraphics();
        RebuildRows();
    }
    else if (action == Action::PostEffects)
    {
        settings.graphics.postEffects = !settings.graphics.postEffects;
        SaveSettings();
    }
    else if (action == Action::ScreenShake)
    {
        settings.gameplay.screenShake = !settings.gameplay.screenShake;
        SaveSettings();
    }
    else if (action == Action::ShowScorePopups)
    {
        settings.gameplay.showScorePopups = !settings.gameplay.showScorePopups;
        SaveSettings();
    }
    else if (action == Action::FrameRateLimit)
    {
        auto iterator{ std::ranges::find(FrameLimits, settings.graphics.frameRateLimit) };
        std::size_t index{ iterator == FrameLimits.end()
            ? 0u
            : static_cast<std::size_t>(std::distance(FrameLimits.begin(), iterator)) };
        index = direction > 0
            ? (index + 1u) % FrameLimits.size()
            : (index == 0u ? FrameLimits.size() - 1u : index - 1u);
        settings.graphics.frameRateLimit = FrameLimits[index];
        SaveAndApplyLiveGraphics();
    }

    RefreshRowTextValues();
}

void OptionsState::HandleMousePosition(sf::Vector2i pixelPosition)
{
    const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pixelPosition) };
    for (std::size_t index{ 0u }; index < rows.size(); ++index)
    {
        if (rows[index].enabled && rows[index].bounds.contains(point))
        {
            Select(index);
            return;
        }
    }
}

void OptionsState::HandleMousePress(sf::Vector2i pixelPosition)
{
    HandleMousePosition(pixelPosition);
    if (!IsSelectedRowEnabled())
        return;

    const sf::Vector2f point{ GetContext().window.mapPixelToCoords(pixelPosition) };
    if (!rows[selectedIndex].bounds.contains(point))
        return;

    if (rows[selectedIndex].kind == RowKind::Dropdown &&
        !GetValueBoxBounds(rows[selectedIndex]).contains(point))
    {
        return;
    }

    if (rows[selectedIndex].kind == RowKind::Slider)
    {
        sliderDragging = true;
        UpdateSliderFromMouse(point);
    }
    else
    {
        ActivateSelected();
    }
}

void OptionsState::UpdateSliderFromMouse(sf::Vector2f position)
{
    if (!IsSelectedRowEnabled())
        return;
    const Action action{ rows[selectedIndex].action };
    if (action != Action::MusicVolume && action != Action::SoundVolume)
        return;

    const float value{ std::clamp((position.x - SliderLeft) / SliderWidth, 0.f, 1.f) * 100.f };
    GameSettings& settings{ GetContext().settings.Edit() };
    if (action == Action::MusicVolume)
        settings.audio.musicVolume = std::round(value);
    else
        settings.audio.soundVolume = std::round(value);
    SaveAndApplyAudio();
    RefreshRowTextValues();
}

void OptionsState::OpenDropdown(Action action)
{
    dropdownAction = action;
    dropdownIndex = action == Action::Resolution
        ? FindCurrentResolution()
        : static_cast<std::size_t>(GetContext().settings.Get().graphics.windowMode);
    dropdownFirstVisible = 0u;
    dropdownOpen = true;
    dropdownScrollbarDragging = false;
    EnsureDropdownSelectionVisible();
    dropdownGlow.Invalidate();
}

void OptionsState::CloseDropdown()
{
    dropdownOpen = false;
    dropdownScrollbarDragging = false;
}

void OptionsState::MoveDropdownSelection(int direction)
{
    const std::size_t count{ GetDropdownItemCount() };
    if (count == 0u)
        return;

    const std::size_t previous{ dropdownIndex };
    if (direction < 0 && dropdownIndex > 0u)
        --dropdownIndex;
    else if (direction > 0 && dropdownIndex + 1u < count)
        ++dropdownIndex;

    if (dropdownIndex != previous)
    {
        EnsureDropdownSelectionVisible();
        dropdownGlow.Invalidate();
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect,
            SoundGroup::UI,
            100.f,
            1.f,
            SoundPlayback::Restart);
    }
}

void OptionsState::EnsureDropdownSelectionVisible()
{
    const std::size_t count{ GetDropdownItemCount() };
    if (count <= MaximumVisibleDropdownItems)
    {
        dropdownFirstVisible = 0u;
        return;
    }

    if (dropdownIndex < dropdownFirstVisible)
        dropdownFirstVisible = dropdownIndex;
    else if (dropdownIndex >= dropdownFirstVisible + MaximumVisibleDropdownItems)
        dropdownFirstVisible = dropdownIndex - MaximumVisibleDropdownItems + 1u;

    dropdownFirstVisible = std::min(
        dropdownFirstVisible,
        count - MaximumVisibleDropdownItems);
}

void OptionsState::HandleDropdownMouseMove(sf::Vector2f position)
{
    const std::size_t visibleCount{ std::min(MaximumVisibleDropdownItems,
        GetDropdownItemCount() - dropdownFirstVisible) };
    for (std::size_t visibleIndex{ 0u }; visibleIndex < visibleCount; ++visibleIndex)
    {
        if (!GetDropdownItemBounds(visibleIndex).contains(position))
            continue;

        const std::size_t hovered{ dropdownFirstVisible + visibleIndex };
        if (hovered != dropdownIndex)
        {
            dropdownIndex = hovered;
            dropdownGlow.Invalidate();
            GetContext().audio.PlaySound(
                Config::Sound::ItemSelect,
                SoundGroup::UI,
                100.f,
                1.f,
                SoundPlayback::Restart);
        }
        return;
    }
}

void OptionsState::HandleDropdownMousePress(sf::Vector2f position)
{
    const std::size_t count{ GetDropdownItemCount() };
    const std::size_t visibleCount{ std::min(MaximumVisibleDropdownItems,
        count - dropdownFirstVisible) };
    if (count > MaximumVisibleDropdownItems && GetDropdownScrollbarBounds().contains(position))
    {
        dropdownScrollbarDragging = true;
        UpdateDropdownScrollbar(position);
        return;
    }

    for (std::size_t visibleIndex{ 0u }; visibleIndex < visibleCount; ++visibleIndex)
    {
        if (GetDropdownItemBounds(visibleIndex).contains(position))
        {
            dropdownIndex = dropdownFirstVisible + visibleIndex;
            ApplyDropdownSelection();
            return;
        }
    }

    CloseDropdown();
}

void OptionsState::HandleMouseWheel(float delta)
{
    const std::size_t count{ GetDropdownItemCount() };
    if (count <= MaximumVisibleDropdownItems || delta == 0.f)
        return;

    const std::size_t previousSelection{ dropdownIndex };
    const std::size_t maximumFirst{ count - MaximumVisibleDropdownItems };
    if (delta > 0.f && dropdownFirstVisible > 0u)
        --dropdownFirstVisible;
    else if (delta < 0.f && dropdownFirstVisible < maximumFirst)
        ++dropdownFirstVisible;

    dropdownIndex = std::clamp(
        dropdownIndex,
        dropdownFirstVisible,
        dropdownFirstVisible + MaximumVisibleDropdownItems - 1u);
    dropdownGlow.Invalidate();
    if (dropdownIndex != previousSelection)
    {
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect,
            SoundGroup::UI,
            100.f,
            1.f,
            SoundPlayback::Restart);
    }
}

void OptionsState::UpdateDropdownScrollbar(sf::Vector2f position)
{
    const std::size_t count{ GetDropdownItemCount() };
    if (count <= MaximumVisibleDropdownItems)
        return;

    const std::size_t previousSelection{ dropdownIndex };
    const sf::FloatRect track{ GetDropdownScrollbarBounds() };
    const float thumbHeight{ track.size.y * static_cast<float>(MaximumVisibleDropdownItems) /
        static_cast<float>(count) };
    const float travel{ track.size.y - thumbHeight };
    const float normalized{ travel <= 0.f
        ? 0.f
        : std::clamp((position.y - track.position.y - thumbHeight * 0.5f) / travel, 0.f, 1.f) };
    dropdownFirstVisible = static_cast<std::size_t>(std::round(
        normalized * static_cast<float>(count - MaximumVisibleDropdownItems)));
    dropdownIndex = std::clamp(
        dropdownIndex,
        dropdownFirstVisible,
        dropdownFirstVisible + MaximumVisibleDropdownItems - 1u);
    dropdownGlow.Invalidate();
    if (dropdownIndex != previousSelection)
    {
        GetContext().audio.PlaySound(
            Config::Sound::ItemSelect,
            SoundGroup::UI,
            100.f,
            1.f,
            SoundPlayback::Restart);
    }
}

void OptionsState::ApplyDropdownSelection()
{
    GetContext().audio.PlaySound(
        Config::Sound::ItemPress,
        SoundGroup::UI,
        100.f,
        1.f,
        SoundPlayback::Restart);

    if (dropdownAction == Action::Resolution)
        ApplyResolution(dropdownIndex);
    else if (dropdownAction == Action::WindowMode)
        ApplyWindowMode(static_cast<WindowMode>(dropdownIndex));
    else
        CloseDropdown();
}

void OptionsState::Execute(Action action)
{
    switch (action)
    {
    case Action::OpenGraphics:
        BeginPageTransition(Page::Graphics);
        break;
    case Action::OpenAudio:
        BeginPageTransition(Page::Audio);
        break;
    case Action::OpenGameplay:
        BeginPageTransition(Page::Gameplay);
        break;
    case Action::OpenControls:
        BeginPageTransition(Page::Controls);
        break;
    case Action::OpenKeyboardControls:
        BeginPageTransition(Page::KeyboardControls);
        break;
    case Action::OpenGamepadControls:
        BeginPageTransition(Page::GamepadControls);
        break;
    case Action::Back:
        if (page == Page::Root)
            BeginExit();
        else if (page == Page::KeyboardControls || page == Page::GamepadControls)
            BeginPageTransition(Page::Controls);
        else
            BeginPageTransition(Page::Root);
        break;
    case Action::ResetAll:
    {
        const GraphicsSettings previous{ GetContext().settings.Get().graphics };
        GetContext().settings.Edit() = GetContext().settings.GetDefaults();
        SaveAndApplyAudio();
        if (RequiresWindowRecreation(previous, GetContext().settings.Get().graphics))
            BeginDisplayChange(previous);
        else
            SaveAndApplyLiveGraphics();
        RebuildRows();
        break;
    }
    case Action::ResetGraphics:
    {
        const GraphicsSettings previous{ GetContext().settings.Get().graphics };
        GetContext().settings.Edit().graphics = GetContext().settings.GetDefaults().graphics;
        if (RequiresWindowRecreation(previous, GetContext().settings.Get().graphics))
            BeginDisplayChange(previous);
        else
            SaveAndApplyLiveGraphics();
        RebuildRows();
        break;
    }
    case Action::ResetAudio:
        GetContext().settings.Edit().audio = GetContext().settings.GetDefaults().audio;
        SaveAndApplyAudio();
        RefreshRowTextValues();
        break;
    case Action::ResetGameplay:
        GetContext().settings.Edit().gameplay = GetContext().settings.GetDefaults().gameplay;
        SaveSettings();
        RefreshRowTextValues();
        break;
    case Action::ResetControls:
        GetContext().settings.Edit().controls = GetContext().settings.GetDefaults().controls;
        SaveSettings();
        RefreshRowTextValues();
        break;
    default:
        break;
    }
}

void OptionsState::SaveSettings()
{
    saveFailed = !GetContext().settings.Save();
}

void OptionsState::SaveAndApplyAudio()
{
    SaveSettings();
    GetContext().audio.ApplySettings();
}

void OptionsState::SaveAndApplyLiveGraphics()
{
    SaveSettings();
    GetContext().display.ApplyLiveSettings(GetContext().settings.Get().graphics);
}

void OptionsState::BeginDisplayChange(const GraphicsSettings& previous)
{
    previousGraphics = previous;
    SaveSettings();
    GetContext().display.ApplyDisplaySettings(GetContext().settings.Get().graphics);
    GetContext().window.setMouseCursorVisible(false);
    displayConfirmationRemaining = DisplayConfirmationDuration;
    dialogSelectedIndex = 0u;
    dialogGlow.Invalidate();
    displayConfirmationOpen = true;
}

void OptionsState::SelectDialogOption(std::size_t index, bool playSound)
{
    index = std::min(index, std::size_t{ 1u });
    const bool changed{ dialogSelectedIndex != index };
    dialogSelectedIndex = index;
    if (changed)
        dialogGlow.Invalidate();
    if (changed && playSound)
        GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI);
}

void OptionsState::ActivateDialogOption(std::size_t index)
{
    GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI);
    if (index == 0u)
        ConfirmDisplayChange();
    else
        RevertDisplayChange();
}

void OptionsState::ConfirmDisplayChange()
{
    displayConfirmationOpen = false;
}

void OptionsState::RevertDisplayChange()
{
    GetContext().settings.Edit().graphics = previousGraphics;
    SaveSettings();
    GetContext().display.ApplyDisplaySettings(previousGraphics);
    GetContext().window.setMouseCursorVisible(false);
    displayConfirmationOpen = false;
    RebuildRows();
}

void OptionsState::ApplyResolution(std::size_t resolutionIndex)
{
    const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
    if (resolutionIndex >= resolutions.size())
        return;
    const GraphicsSettings previous{ GetContext().settings.Get().graphics };
    if (previous.resolution == resolutions[resolutionIndex])
    {
        CloseDropdown();
        return;
    }
    GetContext().settings.Edit().graphics.resolution = resolutions[resolutionIndex];
    CloseDropdown();
    BeginDisplayChange(previous);
}

void OptionsState::ApplyWindowMode(WindowMode mode)
{
    const GraphicsSettings previous{ GetContext().settings.Get().graphics };
    if (previous.windowMode == mode)
    {
        CloseDropdown();
        return;
    }

    GetContext().settings.Edit().graphics.windowMode = mode;
    CloseDropdown();
    BeginDisplayChange(previous);
    RebuildRows();
}

bool OptionsState::RequiresWindowRecreation(
    const GraphicsSettings& before,
    const GraphicsSettings& after) const noexcept
{
    if (before.windowMode != after.windowMode)
        return true;

    return before.windowMode != WindowMode::Borderless && before.resolution != after.resolution;
}

void OptionsState::BeginBinding(Action action)
{
    pendingBinding = action;
    dialogGlow.Invalidate();
}

void OptionsState::ApplyBinding(ControlBinding binding)
{
    if (!pendingBinding.has_value())
        return;
    if (ControlBinding* target{ GetBinding(*pendingBinding) })
    {
        const ControlBinding previous{ *target };
        ControlSettings& controls{ GetContext().settings.Edit().controls };
        const std::array<ControlBinding*, 5> allBindings{
            &controls.moveUp,
            &controls.moveDown,
            &controls.moveLeft,
            &controls.moveRight,
            &controls.fire
        };
        const auto matches{ [&binding](const ControlBinding& candidate)
            {
                return candidate.device == binding.device && candidate.code == binding.code;
            } };

        for (ControlBinding* existing : allBindings)
        {
            if (existing != target && matches(*existing))
            {
                *existing = previous;
                break;
            }
        }

        *target = binding;
        SaveSettings();
        RefreshRowTextValues();
    }
    pendingBinding.reset();
}

ControlBinding* OptionsState::GetBinding(Action action)
{
    ControlSettings& controls{ GetContext().settings.Edit().controls };
    switch (action)
    {
    case Action::MoveUp: return &controls.moveUp;
    case Action::MoveDown: return &controls.moveDown;
    case Action::MoveLeft: return &controls.moveLeft;
    case Action::MoveRight: return &controls.moveRight;
    case Action::Fire: return &controls.fire;
    default: return nullptr;
    }
}

const ControlBinding* OptionsState::GetBinding(Action action) const
{
    const ControlSettings& controls{ GetContext().settings.Get().controls };
    switch (action)
    {
    case Action::MoveUp: return &controls.moveUp;
    case Action::MoveDown: return &controls.moveDown;
    case Action::MoveLeft: return &controls.moveLeft;
    case Action::MoveRight: return &controls.moveRight;
    case Action::Fire: return &controls.fire;
    default: return nullptr;
    }
}

std::string OptionsState::GetRowValue(const Row& row) const
{
    const GameSettings& settings{ GetContext().settings.Get() };
    switch (row.action)
    {
    case Action::Resolution:
        if (!row.enabled)
            return "Desktop resolution";
        return std::to_string(settings.graphics.resolution.x) + " x " +
            std::to_string(settings.graphics.resolution.y);
    case Action::WindowMode:
        return WindowModeName(settings.graphics.windowMode);
    case Action::ShowFps:
        return settings.graphics.showFps ? "ON" : "OFF";
    case Action::VerticalSync:
        return settings.graphics.verticalSync ? "ON" : "OFF";
    case Action::PostEffects:
        return settings.graphics.postEffects ? "ON" : "OFF";
    case Action::ScreenShake:
        return settings.gameplay.screenShake ? "ON" : "OFF";
    case Action::ShowScorePopups:
        return settings.gameplay.showScorePopups ? "ON" : "OFF";
    case Action::FrameRateLimit:
        return settings.graphics.frameRateLimit == 0u
            ? "Unlimited"
            : std::to_string(settings.graphics.frameRateLimit);
    case Action::MusicVolume:
        return std::to_string(static_cast<int>(std::round(settings.audio.musicVolume))) + "%";
    case Action::SoundVolume:
        return std::to_string(static_cast<int>(std::round(settings.audio.soundVolume))) + "%";
    default:
        if (const ControlBinding* binding{ GetBinding(row.action) })
            return GetBindingName(*binding);
        return {};
    }
}

std::string OptionsState::GetBindingName(const ControlBinding& binding) const
{
    if (binding.device == InputDevice::Mouse)
        return MouseButtonName(static_cast<sf::Mouse::Button>(binding.code));

    const auto key{ static_cast<sf::Keyboard::Key>(binding.code) };
    return sf::Keyboard::getDescription(sf::Keyboard::delocalize(key)).toAnsiString();
}

std::size_t OptionsState::FindCurrentResolution() const
{
    const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
    const auto iterator{ std::ranges::find(resolutions,
        GetContext().settings.Get().graphics.resolution) };
    return iterator == resolutions.end()
        ? 0u
        : static_cast<std::size_t>(std::distance(resolutions.begin(), iterator));
}

std::size_t OptionsState::GetDropdownItemCount() const
{
    if (dropdownAction == Action::Resolution)
        return GetContext().display.GetSupportedResolutions().size();
    if (dropdownAction == Action::WindowMode)
        return 3u;
    return 0u;
}

std::string OptionsState::GetDropdownItemLabel(std::size_t index) const
{
    if (dropdownAction == Action::Resolution)
    {
        const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
        if (index < resolutions.size())
            return std::to_string(resolutions[index].x) + " x " +
                std::to_string(resolutions[index].y);
    }
    else if (dropdownAction == Action::WindowMode && index < 3u)
    {
        return WindowModeName(static_cast<WindowMode>(index));
    }
    return {};
}

sf::FloatRect OptionsState::GetValueBoxBounds(const Row& row) const
{
    return sf::FloatRect(
        { ValueBoxLeft, row.bounds.position.y + 12.f },
        { ValueBoxWidth, ValueBoxHeight });
}

sf::FloatRect OptionsState::GetDropdownItemBounds(std::size_t visibleIndex) const
{
    const auto activeRow{ std::ranges::find_if(rows, [this](const Row& row)
        {
            return row.action == dropdownAction;
        }) };
    if (activeRow == rows.end())
        return {};

    const sf::FloatRect valueBox{ GetValueBoxBounds(*activeRow) };
    const float itemWidth{ ValueBoxWidth -
        (GetDropdownItemCount() > MaximumVisibleDropdownItems ? 24.f : 0.f) };
    return sf::FloatRect(
        { valueBox.position.x,
            valueBox.position.y + valueBox.size.y + DropdownItemHeight * static_cast<float>(visibleIndex) },
        { itemWidth, DropdownItemHeight });
}

sf::FloatRect OptionsState::GetDropdownScrollbarBounds() const
{
    const auto activeRow{ std::ranges::find_if(rows, [this](const Row& row)
        {
            return row.action == dropdownAction;
        }) };
    if (activeRow == rows.end())
        return {};

    const sf::FloatRect valueBox{ GetValueBoxBounds(*activeRow) };
    const std::size_t visibleCount{ std::min(
        MaximumVisibleDropdownItems,
        GetDropdownItemCount()) };
    return sf::FloatRect(
        { valueBox.position.x + valueBox.size.x - 17.f,
            valueBox.position.y + valueBox.size.y + 8.f },
        { 10.f, DropdownItemHeight * static_cast<float>(visibleCount) - 16.f });
}

bool OptionsState::IsSelectedRowEnabled() const
{
    return selectedIndex < rows.size() && rows[selectedIndex].enabled;
}

void OptionsState::DrawTitle(sf::RenderTarget& target) const
{
    target.draw(titleGlow);
    target.draw(title);
}

void OptionsState::DrawGamepadLayouts(sf::RenderTarget& target)
{
    const auto drawPanel{ [this, &target](
        const std::string& heading,
        float top,
        const std::array<Config::Texture, 7>& icons,
        const std::array<std::string, 7>& actions,
        NeonGlow& headingGlow)
    {
        RoundedRectangleShape panel({ 1400.f, 310.f }, 18.f, 12u);
        panel.setPosition({ 260.f, top });
        panel.setFillColor(sf::Color(3, 15, 28, 235));
        panel.setOutlineColor(sf::Color(40, 132, 160, 205));
        panel.setOutlineThickness(2.f);
        target.draw(panel);

        sf::Text headingText(
            GetContext().assets.Fonts().Get(Config::Font::MenuSemibold), heading, 34u);
        const sf::FloatRect headingLocalBounds{ headingText.getLocalBounds() };
        headingText.setOrigin({
            headingLocalBounds.position.x + headingLocalBounds.size.x * 0.5f,
            headingLocalBounds.position.y + headingLocalBounds.size.y * 0.5f });
        headingText.setPosition({ 960.f, top + 37.f });
        headingText.setFillColor(BrightCyan);
        headingText.setOutlineColor(sf::Color(3, 18, 31, 240));
        headingText.setOutlineThickness(2.f);
        const sf::FloatRect headingBounds{ headingText.getGlobalBounds() };
        headingGlow.DrawBloom(
            target,
            headingBounds,
            [&headingText](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
            {
                glowTarget.draw(headingText, states);
            },
            Cyan,
            false);
        target.draw(headingText);
        headingGlow.DrawHighlight(target, headingBounds, Cyan);

        sf::RectangleShape dividerGlow({ 1300.f, 7.f });
        dividerGlow.setPosition({ 310.f, top + 71.f });
        dividerGlow.setFillColor(sf::Color(255, 166, 42, 38));
        target.draw(dividerGlow);
        sf::RectangleShape divider({ 1300.f, 2.f });
        divider.setPosition({ 310.f, top + 73.f });
        divider.setFillColor(Orange);
        target.draw(divider);

        for (std::size_t index{ 0u }; index < icons.size(); ++index)
        {
            const bool rightColumn{ index >= 4u };
            const std::size_t row{ rightColumn ? index - 4u : index };
            const sf::Vector2f cardPosition{
                rightColumn ? 980.f : 300.f,
                top + 86.f + static_cast<float>(row) * 51.f };
            constexpr sf::Vector2f CardSize{ 640.f, 46.f };

            RoundedRectangleShape card(CardSize, 10.f, 8u);
            card.setPosition(cardPosition);
            card.setFillColor(sf::Color(5, 27, 43, 218));
            card.setOutlineColor(sf::Color(45, 126, 151, 180));
            card.setOutlineThickness(1.f);
            target.draw(card);

            const sf::Texture& texture{ GetContext().assets.Textures().Get(icons[index]) };
            sf::Sprite icon(texture);
            const sf::Vector2u size{ texture.getSize() };
            const float scale{ std::min(
                70.f / static_cast<float>(size.x),
                42.f / static_cast<float>(size.y)) };
            icon.setOrigin({
                static_cast<float>(size.x) * 0.5f,
                static_cast<float>(size.y) * 0.5f });
            icon.setScale({ scale, scale });
            icon.setPosition({ cardPosition.x + 48.f, cardPosition.y + CardSize.y * 0.5f });
            target.draw(icon);

            DrawText(target, actions[index],
                { cardPosition.x + 100.f, cardPosition.y + 7.f }, 25u, BrightCyan);
        }
    } };

    const std::array<Config::Texture, 7> xboxIcons{
        Config::Texture::XboxLeftStick,
        Config::Texture::XboxRightStick,
        Config::Texture::XboxRightTrigger,
        Config::Texture::XboxDpad,
        Config::Texture::XboxConfirm,
        Config::Texture::XboxBack,
        Config::Texture::XboxMenu
    };
    const std::array<Config::Texture, 7> playStationIcons{
        Config::Texture::PlayStationLeftStick,
        Config::Texture::PlayStationRightStick,
        Config::Texture::PlayStationRightTrigger,
        Config::Texture::PlayStationDpad,
        Config::Texture::PlayStationConfirm,
        Config::Texture::PlayStationBack,
        Config::Texture::PlayStationOptions
    };
    const std::array<std::string, 7> actions{
        "Move", "Aim", "Fire", "Menu Navigation", "Confirm", "Back", "Pause"
    };

    drawPanel("XBOX CONTROLLER", 190.f, xboxIcons, actions, xboxHeadingGlow);
    drawPanel("PLAYSTATION CONTROLLER", 515.f, playStationIcons, actions,
        playStationHeadingGlow);
}

void OptionsState::DrawRows(sf::RenderTarget& target)
{
    if (!displayConfirmationOpen && !pendingBinding.has_value() && IsSelectedRowEnabled())
    {
        const Row& selectedRow{ rows[selectedIndex] };
        neonGlow.DrawBloom(
            target,
            selectedRow.bounds,
            [this, &selectedRow](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
            {
                DrawRow(glowTarget, selectedRow, selectedIndex, states);
            },
            SelectionGlowColor);
    }

    for (std::size_t index{ 0u }; index < rows.size(); ++index)
        DrawRow(target, rows[index], index, sf::RenderStates::Default);

    if (!displayConfirmationOpen && !pendingBinding.has_value() && IsSelectedRowEnabled())
        neonGlow.DrawHighlight(target, rows[selectedIndex].bounds, SelectionGlowColor);

    if (saveFailed)
    {
        DrawText(target, "Settings could not be saved. Check folder permissions.",
            { 570.f, 930.f }, 23, Red);
    }
}

void OptionsState::DrawRow(
    sf::RenderTarget& target,
    const Row& row,
    std::size_t index,
    const sf::RenderStates& states)
{
    const bool selected{ index == selectedIndex && row.enabled };
    RoundedRectangleShape panel(row.bounds.size, 15.f, 10u);
    panel.setPosition(row.bounds.position);
    panel.setFillColor(selected ? sf::Color(8, 34, 48, 226) : sf::Color(5, 17, 29, 210));
    panel.setOutlineColor(selected ? Cyan : sf::Color(52, 76, 92));
    panel.setOutlineThickness(selected ? 2.f : 1.f);
    target.draw(panel, states);

    const sf::Color textColor{ !row.enabled ? Disabled : (selected ? Orange : BrightCyan) };
    rowLabels[index].setFillColor(textColor);
    target.draw(rowLabels[index], states);

    if (row.kind == RowKind::Slider)
    {
        const float value{ row.action == Action::MusicVolume
            ? GetContext().settings.Get().audio.musicVolume
            : GetContext().settings.Get().audio.soundVolume };
        DrawSlider(target, row, value, states);
        rowValues[index].setFillColor(Cyan);
        target.draw(rowValues[index], states);
    }
    else if (row.kind == RowKind::Toggle)
    {
        bool value{ false };
        if (row.action == Action::ShowFps)
            value = GetContext().settings.Get().graphics.showFps;
        else if (row.action == Action::VerticalSync)
            value = GetContext().settings.Get().graphics.verticalSync;
        else if (row.action == Action::PostEffects)
            value = GetContext().settings.Get().graphics.postEffects;
        else if (row.action == Action::ScreenShake)
            value = GetContext().settings.Get().gameplay.screenShake;
        else if (row.action == Action::ShowScorePopups)
            value = GetContext().settings.Get().gameplay.showScorePopups;
        DrawToggle(target, row, value, states);
    }
    else if (row.kind == RowKind::Dropdown)
    {
        const sf::FloatRect bounds{ GetValueBoxBounds(row) };
        RoundedRectangleShape box(bounds.size, 10.f, 8u);
        box.setPosition(bounds.position);
        box.setFillColor(row.enabled ? sf::Color(2, 15, 27, 242) : sf::Color(10, 14, 20, 225));
        box.setOutlineColor(row.enabled ? sf::Color(60, 126, 151) : sf::Color(45, 51, 59));
        box.setOutlineThickness(1.5f);
        target.draw(box, states);

        sf::RectangleShape divider({ 1.f, bounds.size.y - 12.f });
        divider.setPosition({ bounds.position.x + bounds.size.x - 58.f, bounds.position.y + 6.f });
        divider.setFillColor(row.enabled ? sf::Color(60, 126, 151) : sf::Color(45, 51, 59));
        target.draw(divider, states);

        RoundedRectangleShape arrowButton({ 46.f, 46.f }, 7.f, 6u);
        arrowButton.setPosition({ bounds.position.x + bounds.size.x - 52.f, bounds.position.y + 6.f });
        arrowButton.setFillColor(row.enabled ? sf::Color(10, 55, 73, 235) : sf::Color(22, 27, 33, 220));
        arrowButton.setOutlineColor(row.enabled ? sf::Color(75, 170, 196) : sf::Color(48, 55, 63));
        arrowButton.setOutlineThickness(1.f);
        target.draw(arrowButton, states);

        rowValues[index].setFillColor(row.enabled ? Cyan : Disabled);
        target.draw(rowValues[index], states);
        if (!row.enabled)
            target.draw(rowHints[index], states);

        sf::ConvexShape arrow(3u);
        arrow.setPoint(0u, { 0.f, 0.f });
        arrow.setPoint(1u, { 18.f, 0.f });
        arrow.setPoint(2u, { 9.f, 10.f });
        arrow.setPosition({ bounds.position.x + bounds.size.x - 38.f, bounds.position.y + 25.f });
        arrow.setFillColor(row.enabled ? BrightCyan : Disabled);
        target.draw(arrow, states);
    }
    else
    {
        if (!rowValues[index].getString().isEmpty())
        {
            rowValues[index].setFillColor(row.enabled ? Cyan : Disabled);
            target.draw(rowValues[index], states);
        }
    }
}

void OptionsState::DrawSlider(
    sf::RenderTarget& target,
    const Row& row,
    float value,
    const sf::RenderStates& states) const
{
    sf::RectangleShape track({ SliderWidth, 8.f });
    track.setPosition({ SliderLeft, row.bounds.position.y + 38.f });
    track.setFillColor(sf::Color(52, 70, 83));
    target.draw(track, states);

    sf::RectangleShape fill({ SliderWidth * value / 100.f, 8.f });
    fill.setPosition(track.getPosition());
    fill.setFillColor(Cyan);
    target.draw(fill, states);

    sf::CircleShape knob(13.f);
    knob.setOrigin({ 13.f, 13.f });
    knob.setPosition({ SliderLeft + SliderWidth * value / 100.f, row.bounds.position.y + 42.f });
    knob.setFillColor(BrightCyan);
    knob.setOutlineColor(sf::Color(40, 210, 245, 90));
    knob.setOutlineThickness(6.f);
    target.draw(knob, states);
}

void OptionsState::DrawToggle(
    sf::RenderTarget& target,
    const Row& row,
    bool value,
    const sf::RenderStates& states)
{
    const sf::Vector2f position{ 1280.f, row.bounds.position.y + 17.f };
    RoundedRectangleShape shell({ 270.f, 50.f }, 10.f, 8u);
    shell.setPosition(position);
    shell.setFillColor(sf::Color(3, 13, 23, 230));
    shell.setOutlineColor(sf::Color(55, 93, 111));
    shell.setOutlineThickness(1.f);
    target.draw(shell, states);

    RoundedRectangleShape active({ 128.f, 42.f }, 8.f, 8u);
    active.setPosition(position + sf::Vector2f{ value ? 4.f : 138.f, 4.f });
    active.setFillColor(sf::Color(18, 132, 157, 150));
    active.setOutlineColor(Cyan);
    active.setOutlineThickness(1.f);
    target.draw(active, states);
    toggleOnText.setPosition(position + sf::Vector2f{ 44.f, 10.f });
    toggleOnText.setFillColor(value ? BrightCyan : Muted);
    target.draw(toggleOnText, states);
    toggleOffText.setPosition(position + sf::Vector2f{ 178.f, 10.f });
    toggleOffText.setFillColor(value ? Muted : BrightCyan);
    target.draw(toggleOffText, states);
}

void OptionsState::DrawDropdown(sf::RenderTarget& target)
{
    const std::size_t itemCount{ GetDropdownItemCount() };
    if (itemCount == 0u)
        return;

    const std::size_t visibleCount{ std::min(
        MaximumVisibleDropdownItems,
        itemCount - dropdownFirstVisible) };
    const std::size_t selectedVisibleIndex{ dropdownIndex - dropdownFirstVisible };
    const sf::FloatRect selectedBounds{ GetDropdownItemBounds(selectedVisibleIndex) };
    dropdownGlow.DrawBloom(
        target,
        selectedBounds,
        [this, selectedBounds](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
        {
            DrawDropdownItem(glowTarget, selectedBounds, dropdownIndex, true, states);
        },
        SelectionGlowColor);

    for (std::size_t visibleIndex{ 0u }; visibleIndex < visibleCount; ++visibleIndex)
    {
        const std::size_t itemIndex{ dropdownFirstVisible + visibleIndex };
        const sf::FloatRect bounds{ GetDropdownItemBounds(visibleIndex) };
        DrawDropdownItem(
            target,
            bounds,
            itemIndex,
            itemIndex == dropdownIndex,
            sf::RenderStates::Default);
    }

    if (itemCount > MaximumVisibleDropdownItems)
    {
        const sf::FloatRect trackBounds{ GetDropdownScrollbarBounds() };
        RoundedRectangleShape track(trackBounds.size, 5.f, 6u);
        track.setPosition(trackBounds.position);
        track.setFillColor(sf::Color(22, 42, 55, 235));
        target.draw(track);

        const float thumbHeight{ trackBounds.size.y *
            static_cast<float>(MaximumVisibleDropdownItems) / static_cast<float>(itemCount) };
        const float progress{ static_cast<float>(dropdownFirstVisible) /
            static_cast<float>(itemCount - MaximumVisibleDropdownItems) };
        RoundedRectangleShape thumb({ trackBounds.size.x, thumbHeight }, 5.f, 6u);
        thumb.setPosition({ trackBounds.position.x,
            trackBounds.position.y + (trackBounds.size.y - thumbHeight) * progress });
        thumb.setFillColor(Cyan);
        target.draw(thumb);
    }

    dropdownGlow.DrawHighlight(target, selectedBounds, SelectionGlowColor);
}

void OptionsState::DrawDropdownItem(
    sf::RenderTarget& target,
    const sf::FloatRect& bounds,
    std::size_t itemIndex,
    bool selected,
    const sf::RenderStates& states) const
{
    RoundedRectangleShape item(bounds.size, 7.f, 6u);
    item.setPosition(bounds.position);
    item.setFillColor(selected ? sf::Color(12, 58, 76, 250) : sf::Color(3, 16, 28, 248));
    item.setOutlineColor(selected ? Cyan : sf::Color(50, 78, 94));
    item.setOutlineThickness(1.f);
    target.draw(item, states);

    sf::Text label(
        GetContext().assets.Fonts().Get(Config::Font::MenuRegular),
        GetDropdownItemLabel(itemIndex),
        24);
    label.setPosition(bounds.position + sf::Vector2f{ 20.f, 13.f });
    label.setFillColor(selected ? Orange : BrightCyan);
    target.draw(label, states);
}

void OptionsState::DrawDialog(sf::RenderTarget& target)
{
    sf::RectangleShape veil(GetContext().logicalSize);
    veil.setFillColor(sf::Color(0, 2, 6, 190));
    target.draw(veil);
    RoundedRectangleShape dialog({ 900.f, 260.f }, 22.f, 12u);
    dialog.setPosition({ 510.f, 410.f });
    dialog.setFillColor(sf::Color(4, 19, 31, 248));
    dialog.setOutlineColor(Cyan);
    dialog.setOutlineThickness(2.f);
    target.draw(dialog);

    if (displayConfirmationOpen)
    {
        DrawCenteredText(target, "Keep these display settings?", 960.f, 465.f, 36, BrightCyan);
        DrawCenteredText(target, "Reverting in " +
            std::to_string(static_cast<int>(std::ceil(displayConfirmationRemaining))) + " seconds",
            960.f, 520.f, 24, Orange);

        const std::array<std::pair<sf::FloatRect, std::string>, 2> buttons{
            std::pair{ DialogConfirmBounds, std::string{ "Confirm" } },
            std::pair{ DialogCancelBounds, std::string{ "Cancel" } }
        };
        const auto& [selectedBounds, selectedLabel]{ buttons[dialogSelectedIndex] };
        dialogGlow.DrawBloom(
            target,
            selectedBounds,
            [this, &selectedBounds, &selectedLabel](
                sf::RenderTarget& glowTarget,
                const sf::RenderStates& states)
            {
                DrawDialogButton(
                    glowTarget,
                    selectedBounds,
                    selectedLabel,
                    true,
                    states);
            },
            SelectionGlowColor);
        for (std::size_t index{ 0u }; index < buttons.size(); ++index)
        {
            const auto& [bounds, label]{ buttons[index] };
            DrawDialogButton(
                target,
                bounds,
                label,
                index == dialogSelectedIndex,
                sf::RenderStates::Default);
        }
        dialogGlow.DrawHighlight(target, selectedBounds, SelectionGlowColor);
    }
    else
    {
        DrawCenteredText(target, "Press a key or mouse button", 960.f, 475.f, 34, BrightCyan);
        const std::string cancelLabel{ "Cancel" };
        dialogGlow.DrawBloom(
            target,
            BindingCancelBounds,
            [this, &cancelLabel](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
            {
                DrawDialogButton(
                    glowTarget,
                    BindingCancelBounds,
                    cancelLabel,
                    true,
                    states);
            },
            SelectionGlowColor);
        DrawDialogButton(
            target,
            BindingCancelBounds,
            cancelLabel,
            true,
            sf::RenderStates::Default);
        dialogGlow.DrawHighlight(target, BindingCancelBounds, SelectionGlowColor);
    }
}

void OptionsState::DrawDialogButton(
    sf::RenderTarget& target,
    const sf::FloatRect& bounds,
    const std::string& labelValue,
    bool selected,
    const sf::RenderStates& states) const
{
    RoundedRectangleShape button(bounds.size, 12.f, 8u);
    button.setPosition(bounds.position);
    button.setFillColor(selected ? sf::Color(7, 39, 54, 245) : sf::Color(5, 20, 31, 245));
    button.setOutlineColor(selected ? BrightCyan : sf::Color(54, 91, 108));
    button.setOutlineThickness(selected ? 2.f : 1.f);
    target.draw(button, states);

    sf::Text label(
        GetContext().assets.Fonts().Get(Config::Font::MenuRegular),
        labelValue,
        25);
    const sf::FloatRect textBounds{ label.getLocalBounds() };
    label.setOrigin({
        textBounds.position.x + textBounds.size.x * 0.5f,
        textBounds.position.y
    });
    label.setPosition({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + 13.f });
    label.setFillColor(selected ? Orange : BrightCyan);
    target.draw(label, states);
}

void OptionsState::DrawCenteredText(
    sf::RenderTarget& target,
    const std::string& value,
    float centerX,
    float y,
    unsigned int size,
    sf::Color color) const
{
    sf::Text text(GetContext().assets.Fonts().Get(Config::Font::MenuRegular), value, size);
    const sf::FloatRect bounds{ text.getLocalBounds() };
    text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y });
    text.setPosition({ centerX, y });
    text.setFillColor(color);
    target.draw(text);
}

void OptionsState::DrawText(
    sf::RenderTarget& target,
    const std::string& value,
    sf::Vector2f position,
    unsigned int size,
    sf::Color color) const
{
    sf::Text text(GetContext().assets.Fonts().Get(Config::Font::MenuRegular), value, size);
    text.setPosition(position);
    text.setFillColor(color);
    target.draw(text);
}
