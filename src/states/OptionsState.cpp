#include "OptionsState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "app/DisplayManager.h"
#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "settings/SettingsManager.h"

namespace
{
    constexpr sf::Color Cyan{ 105, 225, 242 };
    constexpr sf::Color BrightCyan{ 205, 250, 255 };
    constexpr sf::Color Orange{ 255, 190, 72 };
    constexpr sf::Color Muted{ 128, 148, 164 };
    constexpr sf::Color Disabled{ 76, 88, 101 };
    constexpr sf::Color Red{ 245, 92, 92 };
    constexpr sf::Vector2f RowPosition{ 260.f, 220.f };
    constexpr sf::Vector2f RowSize{ 1400.f, 82.f };
    constexpr float RowSpacing{ 98.f };
    constexpr float SliderLeft{ 1160.f };
    constexpr float SliderWidth{ 380.f };
    constexpr std::array<unsigned int, 7> FrameLimits{ 0u, 30u, 60u, 120u, 144u, 240u, 360u };

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

OptionsState::OptionsState(StateStack& stateStack, StateContext context)
    : State(stateStack, context)
    , background(context.assets, context.logicalSize)
    , shade(context.logicalSize)
    , titleGlow(context.assets.Fonts().Get(Config::Font::MenuSemibold), "OPTIONS", 76)
    , title(context.assets.Fonts().Get(Config::Font::MenuSemibold), "OPTIONS", 76)
    , previousGraphics(context.settings.Get().graphics)
{
    context.window.setMouseCursorVisible(true);
    context.window.setMouseCursor(context.assets.GetCursor(Config::Cursor::MenuPointer));
    shade.setFillColor(sf::Color(0, 4, 10, 150));

    titleGlow.setFillColor(sf::Color(80, 215, 245, 25));
    titleGlow.setOutlineColor(sf::Color(45, 205, 245, 85));
    titleGlow.setOutlineThickness(8.f);
    title.setFillColor(BrightCyan);
    title.setOutlineColor(sf::Color(2, 14, 25, 235));
    title.setOutlineThickness(3.f);
    SetPage(Page::Root);
}

void OptionsState::HandleEvent(const sf::Event& event)
{
    if (displayConfirmationOpen)
    {
        if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
        {
            if (key->code == sf::Keyboard::Key::Enter)
                ConfirmDisplayChange();
            else if (key->code == sf::Keyboard::Key::Escape)
                RevertDisplayChange();
        }
        return;
    }

    if (pendingBinding.has_value())
    {
        if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
        {
            if (key->code == sf::Keyboard::Key::Escape)
                pendingBinding.reset();
            else if (key->code != sf::Keyboard::Key::Unknown)
                ApplyBinding({ InputDevice::Keyboard, static_cast<int>(key->code) });
        }
        else if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
        {
            ApplyBinding({ InputDevice::Mouse, static_cast<int>(mouse->button) });
        }
        return;
    }

    if (dropdownOpen)
    {
        if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
        {
            const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
            if (key->code == sf::Keyboard::Key::Up && dropdownIndex > 0u)
                --dropdownIndex;
            else if (key->code == sf::Keyboard::Key::Down && dropdownIndex + 1u < resolutions.size())
                ++dropdownIndex;
            else if (key->code == sf::Keyboard::Key::Enter)
                ApplyResolution(dropdownIndex);
            else if (key->code == sf::Keyboard::Key::Escape)
                dropdownOpen = false;
        }
        else if (const auto* mouse{ event.getIf<sf::Event::MouseButtonPressed>() })
        {
            if (mouse->button == sf::Mouse::Button::Left)
            {
                const sf::Vector2f point{ GetContext().window.mapPixelToCoords(mouse->position) };
                const std::size_t first{ dropdownIndex > 2u ? dropdownIndex - 2u : 0u };
                const std::size_t count{ std::min<std::size_t>(6u,
                    GetContext().display.GetSupportedResolutions().size() - first) };
                for (std::size_t index{ 0u }; index < count; ++index)
                {
                    const sf::FloatRect bounds(
                        { 1110.f, 306.f + 58.f * static_cast<float>(index) },
                        { 470.f, 54.f });
                    if (bounds.contains(point))
                    {
                        ApplyResolution(first + index);
                        return;
                    }
                }
                dropdownOpen = false;
            }
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
    DrawRows(window);
    if (dropdownOpen)
        DrawDropdown(window);
    if (pendingBinding.has_value() || displayConfirmationOpen)
        DrawDialog(window);
}

void OptionsState::SetPage(Page newPage)
{
    page = newPage;
    selectedIndex = 0u;
    dropdownOpen = false;
    pendingBinding.reset();
    RebuildRows();
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
        add("Controls", RowKind::Button, Action::OpenControls);
        add("Restore Defaults", RowKind::Button, Action::ResetAll);
        add("Back to Main Menu", RowKind::Button, Action::Back);
        break;
    case Page::Graphics:
        add("Display Resolution", RowKind::Dropdown, Action::Resolution,
            GetContext().settings.Get().graphics.windowMode != WindowMode::Borderless);
        add("Window Mode", RowKind::Choice, Action::WindowMode);
        add("Show FPS", RowKind::Toggle, Action::ShowFps);
        add("Vertical Synchronization", RowKind::Toggle, Action::VerticalSync);
        add("Frame Rate Limit", RowKind::Choice, Action::FrameRateLimit,
            !GetContext().settings.Get().graphics.verticalSync);
        add("Restore Graphics Defaults", RowKind::Button, Action::ResetGraphics);
        add("Back", RowKind::Button, Action::Back);
        break;
    case Page::Audio:
        add("Music", RowKind::Slider, Action::MusicVolume);
        add("Sounds", RowKind::Slider, Action::SoundVolume);
        add("Restore Audio Defaults", RowKind::Button, Action::ResetAudio);
        add("Back", RowKind::Button, Action::Back);
        break;
    case Page::Controls:
        add("Move Up", RowKind::Binding, Action::MoveUp);
        add("Move Down", RowKind::Binding, Action::MoveDown);
        add("Move Left", RowKind::Binding, Action::MoveLeft);
        add("Move Right", RowKind::Binding, Action::MoveRight);
        add("Fire", RowKind::Binding, Action::Fire);
        add("Restore Controls Defaults", RowKind::Button, Action::ResetControls);
        add("Back", RowKind::Button, Action::Back);
        break;
    }

    if (!rows.empty() && !rows[selectedIndex].enabled)
        SelectNext();
}

void OptionsState::Select(std::size_t index, bool playSound)
{
    if (index >= rows.size() || !rows[index].enabled)
        return;
    const bool changed{ selectedIndex != index };
    selectedIndex = index;
    if (changed && playSound)
        GetContext().audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 45.f, 1.f,
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
    GetContext().audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 60.f, 1.f,
        SoundPlayback::Restart);

    const Row& row{ rows[selectedIndex] };
    if (row.kind == RowKind::Toggle || row.kind == RowKind::Choice)
        AdjustSelected(1);
    else if (row.kind == RowKind::Dropdown)
    {
        dropdownIndex = FindCurrentResolution();
        dropdownOpen = true;
    }
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
    else if (action == Action::WindowMode)
    {
        const GraphicsSettings previous{ settings.graphics };
        int index{ static_cast<int>(settings.graphics.windowMode) };
        index = (index + (direction > 0 ? 1 : 2)) % 3;
        settings.graphics.windowMode = static_cast<WindowMode>(index);
        BeginDisplayChange(previous);
        RebuildRows();
    }
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
}

void OptionsState::Execute(Action action)
{
    switch (action)
    {
    case Action::OpenGraphics:
        SetPage(Page::Graphics);
        break;
    case Action::OpenAudio:
        SetPage(Page::Audio);
        break;
    case Action::OpenControls:
        SetPage(Page::Controls);
        break;
    case Action::Back:
        if (page == Page::Root)
            RequestPop();
        else
            SetPage(Page::Root);
        break;
    case Action::ResetAll:
    {
        const GraphicsSettings previous{ GetContext().settings.Get().graphics };
        GetContext().settings.Edit() = GetContext().settings.GetDefaults();
        SaveAndApplyAudio();
        BeginDisplayChange(previous);
        RebuildRows();
        break;
    }
    case Action::ResetGraphics:
    {
        const GraphicsSettings previous{ GetContext().settings.Get().graphics };
        GetContext().settings.Edit().graphics = GetContext().settings.GetDefaults().graphics;
        BeginDisplayChange(previous);
        RebuildRows();
        break;
    }
    case Action::ResetAudio:
        GetContext().settings.Edit().audio = GetContext().settings.GetDefaults().audio;
        SaveAndApplyAudio();
        break;
    case Action::ResetControls:
        GetContext().settings.Edit().controls = GetContext().settings.GetDefaults().controls;
        SaveSettings();
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
    GetContext().window.setMouseCursorVisible(true);
    GetContext().window.setMouseCursor(GetContext().assets.GetCursor(Config::Cursor::MenuPointer));
    displayConfirmationRemaining = DisplayConfirmationDuration;
    displayConfirmationOpen = true;
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
    GetContext().window.setMouseCursorVisible(true);
    GetContext().window.setMouseCursor(GetContext().assets.GetCursor(Config::Cursor::MenuPointer));
    displayConfirmationOpen = false;
    RebuildRows();
}

void OptionsState::ApplyResolution(std::size_t resolutionIndex)
{
    const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
    if (resolutionIndex >= resolutions.size())
        return;
    const GraphicsSettings previous{ GetContext().settings.Get().graphics };
    GetContext().settings.Edit().graphics.resolution = resolutions[resolutionIndex];
    dropdownOpen = false;
    BeginDisplayChange(previous);
}

void OptionsState::BeginBinding(Action action)
{
    pendingBinding = action;
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
        return std::format("{} x {}", settings.graphics.resolution.x, settings.graphics.resolution.y);
    case Action::WindowMode:
        return WindowModeName(settings.graphics.windowMode);
    case Action::ShowFps:
        return settings.graphics.showFps ? "ON" : "OFF";
    case Action::VerticalSync:
        return settings.graphics.verticalSync ? "ON" : "OFF";
    case Action::FrameRateLimit:
        return settings.graphics.frameRateLimit == 0u
            ? "Unlimited"
            : std::to_string(settings.graphics.frameRateLimit);
    case Action::MusicVolume:
        return std::format("{}%", static_cast<int>(std::round(settings.audio.musicVolume)));
    case Action::SoundVolume:
        return std::format("{}%", static_cast<int>(std::round(settings.audio.soundVolume)));
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

bool OptionsState::IsSelectedRowEnabled() const
{
    return selectedIndex < rows.size() && rows[selectedIndex].enabled;
}

void OptionsState::DrawTitle(sf::RenderTarget& target) const
{
    std::string value;
    switch (page)
    {
    case Page::Root: value = "OPTIONS"; break;
    case Page::Graphics: value = "GRAPHICS"; break;
    case Page::Audio: value = "AUDIO"; break;
    case Page::Controls: value = "CONTROLS"; break;
    }

    auto center{ [this, &value](sf::Text text)
        {
            text.setString(value);
            const sf::FloatRect bounds{ text.getLocalBounds() };
            text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
                bounds.position.y + bounds.size.y * 0.5f });
            text.setPosition({ GetContext().logicalSize.x * 0.5f, 105.f });
            return text;
        } };
    target.draw(center(titleGlow));
    target.draw(center(title));
}

void OptionsState::DrawRows(sf::RenderTarget& target) const
{
    for (std::size_t index{ 0u }; index < rows.size(); ++index)
        DrawRow(target, rows[index], index);

    if (page == Page::Graphics &&
        GetContext().settings.Get().graphics.windowMode == WindowMode::Borderless)
    {
        DrawText(target, "Resolution is controlled by the desktop in Borderless mode",
            { 980.f, 296.f }, 20, Red);
    }

    if (saveFailed)
    {
        DrawText(target, "Settings could not be saved. Check folder permissions.",
            { 570.f, 930.f }, 23, Red);
    }
}

void OptionsState::DrawRow(sf::RenderTarget& target, const Row& row, std::size_t index) const
{
    const bool selected{ index == selectedIndex && row.enabled };
    if (selected)
    {
        for (int layer{ 3 }; layer >= 1; --layer)
        {
            sf::RectangleShape glow(row.bounds.size + sf::Vector2f{ 12.f * layer, 8.f * layer });
            glow.setPosition(row.bounds.position - sf::Vector2f{ 6.f * layer, 4.f * layer });
            glow.setFillColor(sf::Color(30, 195, 235, static_cast<std::uint8_t>(8 * layer)));
            target.draw(glow);
        }
    }

    sf::RectangleShape panel(row.bounds.size);
    panel.setPosition(row.bounds.position);
    panel.setFillColor(selected ? sf::Color(8, 34, 48, 226) : sf::Color(5, 17, 29, 210));
    panel.setOutlineColor(selected ? Cyan : sf::Color(52, 76, 92));
    panel.setOutlineThickness(selected ? 2.f : 1.f);
    target.draw(panel);

    const sf::Color textColor{ !row.enabled ? Disabled : (selected ? Orange : BrightCyan) };
    DrawText(target, row.label, row.bounds.position + sf::Vector2f{ 34.f, 20.f }, 30, textColor);

    if (row.kind == RowKind::Slider)
    {
        const float value{ row.action == Action::MusicVolume
            ? GetContext().settings.Get().audio.musicVolume
            : GetContext().settings.Get().audio.soundVolume };
        DrawSlider(target, row, value);
    }
    else if (row.kind == RowKind::Toggle)
    {
        const bool value{ row.action == Action::ShowFps
            ? GetContext().settings.Get().graphics.showFps
            : GetContext().settings.Get().graphics.verticalSync };
        DrawToggle(target, row, value);
    }
    else
    {
        const std::string value{ GetRowValue(row) };
        if (!value.empty())
            DrawText(target, value, { 1160.f, row.bounds.position.y + 20.f }, 28,
                row.enabled ? Cyan : Disabled);
    }
}

void OptionsState::DrawSlider(sf::RenderTarget& target, const Row& row, float value) const
{
    sf::RectangleShape track({ SliderWidth, 8.f });
    track.setPosition({ SliderLeft, row.bounds.position.y + 38.f });
    track.setFillColor(sf::Color(52, 70, 83));
    target.draw(track);

    sf::RectangleShape fill({ SliderWidth * value / 100.f, 8.f });
    fill.setPosition(track.getPosition());
    fill.setFillColor(Cyan);
    target.draw(fill);

    sf::CircleShape knob(13.f);
    knob.setOrigin({ 13.f, 13.f });
    knob.setPosition({ SliderLeft + SliderWidth * value / 100.f, row.bounds.position.y + 42.f });
    knob.setFillColor(BrightCyan);
    knob.setOutlineColor(sf::Color(40, 210, 245, 90));
    knob.setOutlineThickness(6.f);
    target.draw(knob);
    DrawText(target, GetRowValue(row), { 1565.f, row.bounds.position.y + 23.f }, 24, Cyan);
}

void OptionsState::DrawToggle(sf::RenderTarget& target, const Row& row, bool value) const
{
    const sf::Vector2f position{ 1280.f, row.bounds.position.y + 17.f };
    sf::RectangleShape shell({ 270.f, 50.f });
    shell.setPosition(position);
    shell.setFillColor(sf::Color(3, 13, 23, 230));
    shell.setOutlineColor(sf::Color(55, 93, 111));
    shell.setOutlineThickness(1.f);
    target.draw(shell);

    sf::RectangleShape active({ 128.f, 42.f });
    active.setPosition(position + sf::Vector2f{ value ? 4.f : 138.f, 4.f });
    active.setFillColor(sf::Color(18, 132, 157, 150));
    active.setOutlineColor(Cyan);
    active.setOutlineThickness(1.f);
    target.draw(active);
    DrawText(target, "ON", position + sf::Vector2f{ 44.f, 10.f }, 23,
        value ? BrightCyan : Muted);
    DrawText(target, "OFF", position + sf::Vector2f{ 178.f, 10.f }, 23,
        value ? Muted : BrightCyan);
}

void OptionsState::DrawDropdown(sf::RenderTarget& target) const
{
    const auto& resolutions{ GetContext().display.GetSupportedResolutions() };
    if (resolutions.empty())
        return;
    const std::size_t first{ dropdownIndex > 2u ? dropdownIndex - 2u : 0u };
    const std::size_t count{ std::min<std::size_t>(6u, resolutions.size() - first) };
    for (std::size_t index{ 0u }; index < count; ++index)
    {
        const std::size_t resolutionIndex{ first + index };
        sf::RectangleShape item({ 470.f, 54.f });
        item.setPosition({ 1110.f, 306.f + 58.f * static_cast<float>(index) });
        item.setFillColor(resolutionIndex == dropdownIndex
            ? sf::Color(12, 58, 76, 250)
            : sf::Color(3, 16, 28, 248));
        item.setOutlineColor(resolutionIndex == dropdownIndex ? Cyan : sf::Color(50, 78, 94));
        item.setOutlineThickness(1.f);
        target.draw(item);
        DrawText(target,
            std::format("{} x {}", resolutions[resolutionIndex].x, resolutions[resolutionIndex].y),
            item.getPosition() + sf::Vector2f{ 22.f, 12.f }, 24,
            resolutionIndex == dropdownIndex ? Orange : BrightCyan);
    }
}

void OptionsState::DrawDialog(sf::RenderTarget& target) const
{
    sf::RectangleShape veil(GetContext().logicalSize);
    veil.setFillColor(sf::Color(0, 2, 6, 190));
    target.draw(veil);
    sf::RectangleShape dialog({ 900.f, 260.f });
    dialog.setPosition({ 510.f, 410.f });
    dialog.setFillColor(sf::Color(4, 19, 31, 248));
    dialog.setOutlineColor(Cyan);
    dialog.setOutlineThickness(2.f);
    target.draw(dialog);

    if (displayConfirmationOpen)
    {
        DrawText(target, "Keep these display settings?", { 650.f, 465.f }, 36, BrightCyan);
        DrawText(target, std::format("Reverting in {} seconds",
            static_cast<int>(std::ceil(displayConfirmationRemaining))), { 730.f, 530.f }, 26, Orange);
        DrawText(target, "ENTER  Keep     ESC  Revert", { 690.f, 600.f }, 24, Muted);
    }
    else
    {
        DrawText(target, "Press a key or mouse button", { 645.f, 475.f }, 34, BrightCyan);
        DrawText(target, "ESC cancels rebinding", { 760.f, 560.f }, 24, Muted);
    }
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
