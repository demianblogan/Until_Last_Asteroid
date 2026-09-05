#include "BossVisual.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

#include <SFML/Graphics/BlendMode.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "assets/Assets.h"
#include "gameplay/BossEncounter.h"
#include "localization/LocalizationManager.h"
#include "rendering/EnergyShield.h"
#include "utils/ConfigEnums.h"
#include "utils/Pulse.h"
#include "utils/Random.h"

namespace
{
	void ConfigureUniformSprite(sf::Sprite& sprite, float displayedWidth, sf::Vector2f origin)
	{
		const sf::Vector2u size{ sprite.getTexture().getSize() };
		sprite.setOrigin(origin);

		const float scale = displayedWidth / static_cast<float>(size.x);
		sprite.setScale({ scale, scale });
	}

	// Appends a filled circle (as a triangle fan) into a shared Triangles
	// VertexArray, so a whole cluster of small glow dots/markers can be
	// issued as one draw call instead of one target.draw() per dot.
	void AppendFilledCircle(sf::VertexArray& triangles, sf::Vector2f center, float radius, sf::Color color,
		int segments = 12, float startAngleRadians = 0.f)
	{
		const float step = 2.f * std::numbers::pi_v<float> / static_cast<float>(segments);

		for (int index = 0; index < segments; index++)
		{
			const float angleA = startAngleRadians + step * static_cast<float>(index);
			const float angleB = startAngleRadians + step * static_cast<float>(index + 1);

			triangles.append({ center, color });
			triangles.append({ center + sf::Vector2f{ std::cos(angleA) * radius, std::sin(angleA) * radius }, color });
			triangles.append({ center + sf::Vector2f{ std::cos(angleB) * radius, std::sin(angleB) * radius }, color });
		}
	}

	void DrawLightningBolt(sf::RenderTarget& target, const auto& bolt, sf::RenderStates states)
	{
		constexpr int SegmentCount = 18;
		const sf::Vector2f delta{ bolt.end - bolt.start };
		const float length = delta.length();

		if (length <= 0.1f)
			return;

		const sf::Vector2f perpendicular{ (delta / length).perpendicular() };
		const float fade = 1.f - std::clamp(bolt.elapsed / BossEncounter::LightningDuration, 0.f, 1.f);
		const float seed = bolt.start.x * 0.017f + bolt.start.y * 0.031f;

		// The jagged centreline, built once: a straight run from start to end
		// pushed sideways by a per-node jitter that tapers to zero at both ends.
		// All three passes below (glow dots, outline layers, bright core) draw
		// this same polyline -- the outline layers just add a constant sideways
		// offset per layer.
		std::array<sf::Vector2f, SegmentCount + 1> nodes;
		for (int index = 0; index <= SegmentCount; index++)
		{
			const float progress = static_cast<float>(index) / SegmentCount;
			const float envelope = std::sin(progress * std::numbers::pi_v<float>);
			const float jitter = std::sin(seed + index * 8.73f) * 13.f * envelope;

			nodes[index] = bolt.start + delta * progress + perpendicular * jitter;
		}

		sf::RenderStates glowStates{ states };
		glowStates.blendMode = sf::BlendAdd;

		sf::VertexArray glowDots(sf::PrimitiveType::Triangles);
		for (int index = 0; index <= SegmentCount; index += 2)
		{
			AppendFilledCircle(glowDots, nodes[index], 13.f, sf::Color(255, 72, 8, static_cast<std::uint8_t>(38.f * fade)));
			AppendFilledCircle(glowDots, nodes[index], 5.f, sf::Color(255, 190, 72, static_cast<std::uint8_t>(115.f * fade)));
		}
		target.draw(glowDots, glowStates);

		for (int layer = 0; layer < 3; layer++)
		{
			const sf::Vector2f layerOffset{ perpendicular * (static_cast<float>(layer - 1) * 2.f) };
			const auto alpha = static_cast<std::uint8_t>((95.f - layer * 20.f) * fade);

			sf::VertexArray line(sf::PrimitiveType::LineStrip);
			for (const sf::Vector2f& node : nodes)
				line.append({ node + layerOffset, sf::Color(255, 105, 18, alpha) });

			target.draw(line, glowStates);
		}

		sf::VertexArray coreLine(sf::PrimitiveType::LineStrip);
		for (const sf::Vector2f& node : nodes)
			coreLine.append({ node, sf::Color(255, 225, 145, static_cast<std::uint8_t>(255.f * fade)) });

		target.draw(coreLine, glowStates);
	}

	void DrawCoreBeam(sf::RenderTarget& target, sf::Vector2f start, sf::Vector2f end, float beamWidth,
		float pulse, float animationTime, sf::RenderStates states)
	{
		const sf::Vector2f delta{ end - start };
		const float length = delta.length();
		if (length <= 0.1f)
			return;

		const sf::Vector2f direction{ delta / length };
		const sf::Vector2f perpendicular{ direction.perpendicular() };

		states.blendMode = sf::BlendAdd;

		const float outerHalfWidth = beamWidth * 2.8f;
		const float glowHalfWidth = beamWidth * 1.35f;
		const float coreHalfWidth = beamWidth * 0.5f;

		const auto withAlpha = [pulse](sf::Color color, float alpha)
			{
				color.a = static_cast<std::uint8_t>(std::clamp(alpha * pulse, 0.f, 255.f));
				return color;
			};

		const std::array offsets =
		{
			-outerHalfWidth,
			-glowHalfWidth,
			-coreHalfWidth,
			0.f,
			coreHalfWidth,
			glowHalfWidth,
			outerHalfWidth
		};

		const sf::Color orangeGlow{ 255, 78, 8 };

		const std::array colors =
		{
			withAlpha(orangeGlow, 0.f),
			withAlpha(orangeGlow, 38.f),
			withAlpha(orangeGlow, 150.f),
			withAlpha(sf::Color(255, 238, 196), 255.f),
			withAlpha(orangeGlow, 150.f),
			withAlpha(orangeGlow, 38.f),
			withAlpha(orangeGlow, 0.f)
		};

		sf::VertexArray beam(sf::PrimitiveType::TriangleStrip);
		beam.resize(offsets.size() * 2u);

		for (std::size_t index = 0u; index < offsets.size(); index++)
		{
			beam[index * 2u] = sf::Vertex{ start + perpendicular * offsets[index], colors[index] };
			beam[index * 2u + 1u] = sf::Vertex{ end + perpendicular * offsets[index], colors[index] };
		}

		target.draw(beam, states);

		constexpr int MarkerCount = 22;
		sf::VertexArray markers(sf::PrimitiveType::Triangles);

		for (int index = 0; index < MarkerCount; index++)
		{
			const float base = static_cast<float>(index) / MarkerCount;
			const float travel = std::fmod(base + animationTime * 0.7f, 1.f);
			const float phase = travel * 4.f * std::numbers::pi_v<float> +animationTime * 5.f;
			const float offset = std::sin(phase) * beamWidth * 0.8f;
			const float depth = 0.5f + 0.5f * std::cos(phase);
			const float radius = 4.f + depth * 5.f;
			const sf::Vector2f markerPosition{ start + direction * (travel * length) + perpendicular * offset };
			const float rotationRadians = (45.f + animationTime * 150.f) * (std::numbers::pi_v<float> / 180.f);

			AppendFilledCircle(markers, markerPosition, radius, sf::Color(255, 174, 65,
				static_cast<std::uint8_t>(135.f + depth * 120.f)), 4, rotationRadians);
		}

		target.draw(markers, states);
	}

	// Cosmetic screen-shake offset for a boss part while its destruction
	// sequence plays. Kept in the visual layer -- it never affects anything
	// the simulation cares about.
	sf::Vector2f DestructionShake(float magnitude)
	{
		return { Random::Float(-magnitude, magnitude), Random::Float(-magnitude, magnitude) };
	}
}

BossVisual::BossVisual(Assets& assets, LocalizationManager& localization, sf::Vector2f logicalSize)
	: localization(localization)
	, hitFlashShader(assets.GetShader(Config::Shader::HitFlash))
	, hudCenterX(logicalSize.x * 0.5f)
	, outerRing(assets.Textures().Get(Config::Texture::BossOuterRing))
	, diamond(assets.Textures().Get(Config::Texture::BossDiamond))
	, core(assets.Textures().Get(Config::Texture::BossCore))
	, armorLabel(assets.Fonts().Get(localization.GetBoldFont()), "", 22u)
{
	const sf::Vector2u ringSize{ outerRing.getTexture().getSize() };
	ConfigureUniformSprite(outerRing, 650.f, { ringSize.x * 0.5f, ringSize.y * 0.5f });

	const sf::Vector2u diamondSize{ diamond.getTexture().getSize() };
	ConfigureUniformSprite(diamond, 425.f, { diamondSize.x * 0.5f, diamondSize.y * 0.5f });

	// The core sprite's brain silhouette isn't centered in its source image,
	// so the origin is anchored at a specific source pixel rather than the
	// texture's geometric center. That pixel coordinate scales with the
	// texture's own resolution: it was authored against a 1254x1254 source,
	// so scale it the same way for whatever resolution the texture is now.
	const sf::Vector2u coreSize{ core.getTexture().getSize() };
	const float coreOriginScale = static_cast<float>(coreSize.x) / 1254.f;
	ConfigureUniformSprite(core, 280.f, { 628.f * coreOriginScale, 628.f * coreOriginScale });

	const float maskRadius = assets.GetGameplayData().GetBoss().portalCollisionRadius * 0.86f;
	for (sf::CircleShape& mask : destroyedPortalMasks)
	{
		mask.setRadius(maskRadius);
		mask.setOrigin({ mask.getRadius(), mask.getRadius() });
		mask.setFillColor(sf::Color(8, 5, 6, 245));
		mask.setOutlineColor(sf::Color(92, 28, 20, 230));
		mask.setOutlineThickness(3.f);
	}

	armorLabel.setFillColor(sf::Color(255, 238, 232));
	armorLabel.setOutlineColor(sf::Color(80, 0, 0, 230));
	armorLabel.setOutlineThickness(2.f);
}

void BossVisual::Draw(sf::RenderTarget& target, sf::RenderStates states, const BossEncounter& boss) const
{
	using State = BossEncounter::State;

	if (!boss.IsActive())
		return;

	const float flashDuration = boss.hitFlashDuration;

	// Each boss part (ring/diamond/core) briefly tints white when hit. The
	// hit-flash shader is a single shared instance whose uniforms get
	// rewritten per draw, so this returns a fresh RenderStates pointing at
	// it -- configured for `remaining` seconds of flash left -- right before
	// that part is drawn, rather than three copies of the same setup.
	const auto flashStates = [&](float remaining)
		{
			sf::RenderStates result{ states };

			if (remaining > 0.f && flashDuration > 0.f)
			{
				hitFlashShader.setUniform("source", sf::Shader::CurrentTexture);
				hitFlashShader.setUniform("intensity", remaining / flashDuration);
				result.shader = &hitFlashShader;
			}

			return result;
		};

	if (boss.isOuterRingVisible)
	{
		sf::Sprite ring{ outerRing };
		ring.setRotation(sf::degrees(boss.ringRotationDegrees));
		ring.setPosition(boss.position + (boss.state == State::OuterDestroying ? DestructionShake(7.f) : sf::Vector2f{}));
		target.draw(ring, flashStates(boss.ringHitFlashRemaining));
	}

	if (boss.isDiamondVisible)
	{
		sf::Sprite frame{ diamond };
		frame.setRotation(sf::degrees(boss.diamondRotationDegrees));
		frame.setPosition(boss.position + (boss.state == State::InnerDestroying ? DestructionShake(6.f) : sf::Vector2f{}));
		target.draw(frame, flashStates(boss.diamondHitFlashRemaining));

		for (std::size_t index = 0u; index < boss.portalHealth.size(); index++)
		{
			if (boss.portalHealth[index] > 0)
				continue;

			sf::CircleShape mask{ destroyedPortalMasks[index] };
			mask.setPosition(boss.GetPortalPosition(index));
			target.draw(mask, states);
		}
	}

	if (boss.isCoreVisible)
	{
		sf::Sprite brain{ core };
		brain.setPosition(boss.position + BossEncounter::CoreVisualOffset);
		target.draw(brain, flashStates(boss.coreHitFlashRemaining));
	}

	const bool isCoreCombatActive =
		boss.state == State::CoreShieldWarning || boss.state == State::CoreShield || boss.state == State::CoreExposed;

	if (isCoreCombatActive)
	{
		const float pulse = 0.82f + 0.18f * std::abs(std::sin(boss.coreBeamAngleDegrees * 0.12f));
		DrawCoreBeam(target, boss.position + BossEncounter::CoreVisualOffset, boss.GetCoreBeamEnd(),
			boss.config.coreBeamWidth, pulse, boss.coreBeamVisualTime, states);
	}

	const bool isWarningState = boss.state == State::OuterShieldWarning ||
		boss.state == State::InnerShieldWarning || boss.state == State::CoreShieldWarning;

	const bool isWarningShieldVisible = isWarningState &&
		std::fmod(boss.stateElapsed, boss.config.shieldWarningBlinkInterval * 2.f) < boss.config.shieldWarningBlinkInterval;

	if (boss.state == State::Arriving || boss.state == State::ShieldDelay ||
		boss.state == State::OuterShield || boss.state == State::InnerShield || boss.state == State::CoreShield ||
		isWarningShieldVisible)
	{
		const float pulse = Pulse::Value(boss.shieldPulse, 5.f, 0.72f, 0.28f);
		Rendering::DrawEnergyShield(
			target,
			boss.GetCollisionCenter(),
			boss.GetShieldRadius(),
			pulse,
			{ 255, 88, 12 },
			{ 255, 142, 24 },
			{ 255, 105, 16 },
			34.f,
			states);
	}

	for (const BossEncounter::Lightning& bolt : boss.lightning)
		DrawLightningBolt(target, bolt, states);
}

void BossVisual::DrawHud(sf::RenderTarget& target, const BossEncounter& boss) const
{
	using State = BossEncounter::State;

	if (boss.state == State::Dormant || boss.state == State::Arriving || boss.state == State::ShieldDelay)
		return;

	constexpr float BarWidth = 920.f;
	constexpr float BarHeight = 38.f;
	constexpr float Border = 4.f;
	const float ratio = std::clamp(static_cast<float>(boss.health) / static_cast<float>(boss.config.maximumHealth), 0.f, 1.f);

	sf::RectangleShape shadow({ BarWidth + 12.f, BarHeight + 12.f });
	shadow.setPosition({ hudCenterX - (BarWidth + 12.f) * 0.5f, 18.f });
	shadow.setFillColor(sf::Color(0, 0, 0, 175));
	target.draw(shadow);

	sf::RectangleShape frame({ BarWidth, BarHeight });
	frame.setPosition({ hudCenterX - BarWidth * 0.5f, 24.f });
	frame.setFillColor(sf::Color(28, 3, 7, 235));
	frame.setOutlineColor(sf::Color(185, 32, 42, 245));
	frame.setOutlineThickness(2.f);
	target.draw(frame);

	sf::RectangleShape fill({ (BarWidth - Border * 2.f) * ratio, BarHeight - Border * 2.f });
	fill.setPosition({ hudCenterX - BarWidth * 0.5f + Border, 24.f + Border });
	fill.setFillColor(sf::Color(205, 18, 35, 235));
	target.draw(fill);

	const int percent = 
		static_cast<int>(std::ceil(100.f * static_cast<float>(boss.health) / static_cast<float>(boss.config.maximumHealth)));

	sf::Text label{ armorLabel };
	label.setString(localization.FormatText("hud.boss_armor", "value", std::to_string(percent)));

	const sf::FloatRect bounds{ label.getLocalBounds() };
	label.setOrigin({ bounds.position.x + bounds.size.x * 0.5f, bounds.position.y + bounds.size.y * 0.5f });
	label.setPosition({ hudCenterX, 42.f });
	target.draw(label);
}