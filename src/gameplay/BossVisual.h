#pragma once

#include <array>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class RenderTarget;
	class Shader;
}

class Assets;
class BossEncounter;
class LocalizationManager;

// Everything that turns a BossEncounter's current state into pixels: the
// ring/diamond/core sprites and their hit-flash, the destroyed-portal masks,
// the rotating core beam, the energy shield, the retaliation lightning, and
// the HUD health bar. Pulled out of BossEncounter so that class stays purely
// the state machine + combat rules with no rendering mixed in. Reads the
// boss's state directly through friendship (see BossEncounter's `friend`).
class BossVisual
{
public:
	BossVisual(Assets& assets, LocalizationManager& localization, sf::Vector2f logicalSize);

	void Draw(sf::RenderTarget& target, sf::RenderStates states, const BossEncounter& boss) const;
	void DrawHud(sf::RenderTarget& target, const BossEncounter& boss) const;

private:
	LocalizationManager& localization;
	sf::Shader& hitFlashShader;
	float hudCenterX;

	// Un-positioned "templates" -- Draw() stamps positioned/rotated copies
	// each frame (same trick the destroyed-portal masks already used), which
	// keeps Draw() const without any mutable members.
	sf::Sprite outerRing;
	sf::Sprite diamond;
	sf::Sprite core;
	sf::Text armorLabel;
	std::array<sf::CircleShape, 4> destroyedPortalMasks;
};
