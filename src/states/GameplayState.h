#pragma once

#include <optional>
#include <vector>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

#include "core/World.h"
#include "game/GameplayData.h"
#include "game/GameplaySession.h"
#include "rendering/GameplayBackground.h"
#include "rendering/GameplayEffects.h"
#include "states/State.h"
#include "systems/ActionMap.h"
#include "systems/InputHandler.h"
#include "ui/GlowingCursor.h"
#include "ui/HUD.h"
#include "utils/ConfigEnums.h"

class GameplayState final : public State
{
public:
	GameplayState(StateStack& stateStack, StateContext context);
	~GameplayState() override;

	void HandleEvent(const sf::Event& event) override;
	void HandleRealtime() override;
	void Update(float deltaTime) override;
	void Render() override;
	void RenderOverlay() override;

private:
	struct RuntimeWave
	{
		GameplayData::WaveConfig config;
		float timer{ 0.f };
		int repetitionsSpawned{ 0 };
	};

	void SetupInput();
	void SetupUI();
	void OpenPauseMenu();
	void ResumeGameplaySounds();
	void SpawnPlayerIfNeeded();
	void SpawnConfiguredEnemy(GameplayData::EnemyKind kind);
	void Reset();
	void NextLevel();
	void SpawnLevel();
	void CenterTextX(sf::Text& text);
	void CenterText(sf::Text& text, float y);
	[[nodiscard]] sf::Vector2f GetSafeSpawnPosition();
	[[nodiscard]] sf::Vector2f GetSafeEdgeSpawnPosition();

	static constexpr float SpawnSafeRadius{ 250.f };

	const GameplayData& gameplayData;
	GameplaySession session;
	ActionMap<Config::PlayerAction> actions;
	InputHandler<Config::PlayerAction> input;
	GameplayBackground background;
	GameplayEffects effects;
	World world;
	GlowingCursor crosshair;
	std::optional<HUD> hud;
	std::vector<sf::Text> gameOverTexts;
	std::vector<sf::Text> levelCompleteTexts;
	std::vector<sf::Text> winTexts;
	std::optional<sf::Text> exitHintText;
	std::vector<RuntimeWave> currentWaves;
	bool gameplaySoundsPaused{ false };
};
