#include "ResultScreen.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

#include "assets/AssetStore.h"
#include "audio/AudioManager.h"
#include "systems/GamepadManager.h"
#include "utils/ConfigEnums.h"

namespace
{
    constexpr sf::Vector2f TitleFrameSize{ 1180.f, 203.f };
    constexpr sf::Vector2f ButtonSize{ 540.f, 104.f };
    constexpr float DarkenDuration{ 0.42f };
    constexpr float TitleStart{ 0.22f };
    constexpr float TitleDuration{ 0.28f };
    constexpr float SummaryStart{ 0.54f };
    constexpr float SummaryDuration{ 0.22f };
    constexpr float FirstButtonStart{ 0.78f };
    constexpr float SecondButtonStart{ 0.94f };
    constexpr float InteractiveTime{ 1.08f };
    constexpr sf::Color SuccessGlowColor{ 65, 255, 90 };
    constexpr sf::Color SelectionGlowColor{ 255, 178, 42 };

    float Progress(float elapsed, float start, float duration)
    {
        return std::clamp((elapsed - start) / duration, 0.f, 1.f);
    }

    std::uint8_t ToAlpha(float opacity)
    {
        return static_cast<std::uint8_t>(std::clamp(opacity, 0.f, 1.f) * 255.f);
    }
}

ResultScreen::ResultScreen(
    AssetStore& assets, AudioManager& gameAudio, GamepadManager& gamepadManager,
    sf::Vector2f screenSize)
    : audio(gameAudio)
    , gamepad(gamepadManager)
    , logicalSize(screenSize)
    , shade(screenSize)
    , titleFrame(assets.Textures().Get(Config::Texture::ResultTitleFrame))
    , title(assets.Fonts().Get(Config::Font::MenuSemibold), "LEVEL COMPLETE", 86u)
    , summary(assets.Fonts().Get(Config::Font::MenuRegular), "Score: 0", 36u)
    , titleGlow(assets)
    , buttonGlow(assets)
    , menuCursor(assets, Config::Texture::MenuPointer, { 6.f, 2.f }, SelectionGlowColor)
{
    const sf::Vector2u textureSize{ titleFrame.getTexture().getSize() };
    titleFrame.setScale({
        TitleFrameSize.x / static_cast<float>(textureSize.x),
        TitleFrameSize.y / static_cast<float>(textureSize.y) });
    titleFrame.setPosition({ (logicalSize.x - TitleFrameSize.x) * 0.5f, 205.f });

    title.setFillColor(sf::Color(225, 255, 230));
    title.setOutlineColor(sf::Color(0, 92, 30));
    title.setOutlineThickness(4.f);
    title.setLetterSpacing(1.05f);
    CenterText(title, { logicalSize.x * 0.5f, 307.f });

    summary.setFillColor(sf::Color(225, 245, 250));
    summary.setOutlineColor(sf::Color(2, 14, 25, 230));
    summary.setOutlineThickness(2.f);
    CenterText(summary, { logicalSize.x * 0.5f, 455.f });

    const sf::Font& menuFont{ assets.Fonts().Get(Config::Font::MenuRegular) };
    const sf::Texture& idle{ assets.Textures().Get(Config::Texture::MenuButtonIdle) };
    const sf::Texture& selected{ assets.Textures().Get(Config::Texture::MenuButtonSelected) };
    buttons.reserve(2u);
    for (std::size_t index{ 0u }; index < 2u; ++index)
    {
        buttons.emplace_back(menuFont, idle, selected, "", ButtonSize);
        buttons.back().SetPosition({
            (logicalSize.x - ButtonSize.x) * 0.5f,
            520.f + static_cast<float>(index) * 120.f });
    }

    Reset();
}

void ResultScreen::Start(Mode newMode, int level, int score)
{
    mode = newMode;
    active = true;
    interactive = false;
    elapsed = 0.f;
    selectedIndex = 0u;

    if (mode == Mode::Victory)
    {
        title.setString("VICTORY");
        summary.setString("Final Score: " + std::to_string(score));
        buttons[0].SetLabel("Play Again");
    }
    else
    {
        title.setString("LEVEL " + std::to_string(level) + " COMPLETE");
        summary.setString("Score: " + std::to_string(score));
        buttons[0].SetLabel("Continue");
    }
    buttons[1].SetLabel("Go to Main Menu");
    CenterText(title, { logicalSize.x * 0.5f, 307.f });
    CenterText(summary, { logicalSize.x * 0.5f, 455.f });
    Select(0u, false);
    titleGlow.Invalidate();
    buttonGlow.Invalidate();
    ApplyVisualState();
    audio.PlaySound(Config::Sound::InterfaceActivation, SoundGroup::UI, 80.f, 1.f,
        SoundPlayback::Restart);
}

void ResultScreen::Reset()
{
    active = false;
    interactive = false;
    elapsed = 0.f;
    shade.setFillColor(sf::Color::Transparent);
    titleFrame.setColor(sf::Color::Transparent);
    title.setFillColor(sf::Color::Transparent);
    title.setOutlineColor(sf::Color::Transparent);
    summary.setFillColor(sf::Color::Transparent);
    summary.setOutlineColor(sf::Color::Transparent);
}

void ResultScreen::Update(float deltaTime)
{
    if (!active)
        return;
    titleGlow.Update(deltaTime);
    buttonGlow.Update(deltaTime);
    menuCursor.Update(deltaTime);
    elapsed = std::min(InteractiveTime, elapsed + deltaTime);
    interactive = elapsed >= InteractiveTime;
    ApplyVisualState();
}

std::optional<ResultScreen::Action> ResultScreen::HandleEvent(
    const sf::Event& event, sf::RenderWindow& window)
{
    if (!active)
        return std::nullopt;

    const auto navigation{ gamepad.GetNavigationAction(event) };
    if (!interactive)
    {
        if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>() ||
            navigation != GamepadManager::NavigationAction::None)
            SkipAnimation();
        return std::nullopt;
    }

    using enum GamepadManager::NavigationAction;
    switch (navigation)
    {
    case Up: SelectPrevious(); return std::nullopt;
    case Down: SelectNext(); return std::nullopt;
    case Confirm: return ActivateSelected();
    case Back: Select(1u, false); return ActivateSelected();
    default: break;
    }

    if (const auto* key{ event.getIf<sf::Event::KeyPressed>() })
    {
        if (key->code == sf::Keyboard::Key::Up || key->code == sf::Keyboard::Key::W)
            SelectPrevious();
        else if (key->code == sf::Keyboard::Key::Down || key->code == sf::Keyboard::Key::S)
            SelectNext();
        else if (key->code == sf::Keyboard::Key::Enter || key->code == sf::Keyboard::Key::Space)
            return ActivateSelected();
        return std::nullopt;
    }

    if (const auto* moved{ event.getIf<sf::Event::MouseMoved>() })
    {
        UpdateMouseSelection(window.mapPixelToCoords(moved->position));
        return std::nullopt;
    }
    if (const auto* pressed{ event.getIf<sf::Event::MouseButtonPressed>() })
    {
        if (pressed->button != sf::Mouse::Button::Left)
            return std::nullopt;
        const sf::Vector2f position{ window.mapPixelToCoords(pressed->position) };
        for (std::size_t index{ 0u }; index < buttons.size(); ++index)
            if (buttons[index].Contains(position))
            {
                Select(index, false);
                return ActivateSelected();
            }
    }
    return std::nullopt;
}

void ResultScreen::Draw(sf::RenderTarget& target)
{
    if (!active)
        return;

    target.draw(shade);
    if (Progress(elapsed, TitleStart, TitleDuration) >= 1.f)
    {
        titleGlow.DrawBloom(target, titleFrame.getGlobalBounds(),
            [this](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
            {
                glowTarget.draw(titleFrame, states);
                glowTarget.draw(title, states);
            }, SuccessGlowColor, false);
    }
    target.draw(titleFrame);
    target.draw(title);
    target.draw(summary);

    const std::size_t visibleButtons{ elapsed >= SecondButtonStart
        ? 2u : (elapsed >= FirstButtonStart ? 1u : 0u) };
    if (interactive && visibleButtons > 0u)
    {
        const MenuButton& selected{ buttons[selectedIndex] };
        buttonGlow.DrawBloom(target, selected.GetBounds(),
            [&selected](sf::RenderTarget& glowTarget, const sf::RenderStates& states)
            { selected.Draw(glowTarget, states); }, SelectionGlowColor);
    }
    for (std::size_t index{ 0u }; index < visibleButtons; ++index)
        buttons[index].Draw(target);
    if (interactive && visibleButtons > 0u)
        buttonGlow.DrawHighlight(target, buttons[selectedIndex].GetBounds(), SelectionGlowColor);
}

void ResultScreen::DrawCursor(sf::RenderWindow& window)
{
    if (active && interactive && !gamepad.IsUsingGamepad())
        menuCursor.Draw(window);
}

bool ResultScreen::IsActive() const noexcept { return active; }
ResultScreen::Mode ResultScreen::GetMode() const noexcept { return mode; }

void ResultScreen::SkipAnimation()
{
    elapsed = InteractiveTime;
    interactive = true;
    ApplyVisualState();
    Select(0u, false);
}

void ResultScreen::ApplyVisualState()
{
    shade.setFillColor(sf::Color(0, 3, 12,
        ToAlpha(Progress(elapsed, 0.f, DarkenDuration) * 0.76f)));
    const auto titleAlpha{ ToAlpha(Progress(elapsed, TitleStart, TitleDuration)) };
    titleFrame.setColor(sf::Color(255, 255, 255, titleAlpha));
    title.setFillColor(sf::Color(225, 255, 230, titleAlpha));
    title.setOutlineColor(sf::Color(0, 92, 30, titleAlpha));
    const auto summaryAlpha{ ToAlpha(Progress(elapsed, SummaryStart, SummaryDuration)) };
    summary.setFillColor(sf::Color(225, 245, 250, summaryAlpha));
    summary.setOutlineColor(sf::Color(2, 14, 25, summaryAlpha));
}

void ResultScreen::Select(std::size_t index, bool playSound)
{
    const bool changed{ selectedIndex != index };
    selectedIndex = index;
    for (std::size_t i{ 0u }; i < buttons.size(); ++i)
        buttons[i].SetSelected(i == selectedIndex);
    if (changed)
        buttonGlow.Invalidate();
    if (changed && playSound)
        audio.PlaySound(Config::Sound::ItemSelect, SoundGroup::UI, 100.f, 1.f,
            SoundPlayback::Restart);
}

void ResultScreen::SelectPrevious()
{
    Select(selectedIndex == 0u ? buttons.size() - 1u : selectedIndex - 1u);
}

void ResultScreen::SelectNext() { Select((selectedIndex + 1u) % buttons.size()); }

void ResultScreen::UpdateMouseSelection(sf::Vector2f position)
{
    for (std::size_t index{ 0u }; index < buttons.size(); ++index)
        if (buttons[index].Contains(position))
        {
            Select(index);
            return;
        }
}

std::optional<ResultScreen::Action> ResultScreen::ActivateSelected()
{
    audio.PlaySound(Config::Sound::ItemPress, SoundGroup::UI, 100.f, 1.f,
        SoundPlayback::Restart);
    return selectedIndex == 0u ? Action::Primary : Action::MainMenu;
}

void ResultScreen::CenterText(sf::Text& text, sf::Vector2f position)
{
    const sf::FloatRect bounds{ text.getLocalBounds() };
    text.setOrigin({ bounds.position.x + bounds.size.x * 0.5f,
        bounds.position.y + bounds.size.y * 0.5f });
    text.setPosition(position);
}
