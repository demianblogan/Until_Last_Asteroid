#pragma once

#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
	class Drawable;
	class RenderTexture;
	class RenderWindow;
	class Shader;
}

namespace Rendering
{
	// The window's actual drawable area in pixels (its viewport, or the full
	// window size if no viewport/view is set), floored to at least 1x1 so it's
	// always safe to pass to RenderTexture::resize. Shared by every screen that
	// sizes an off-screen render texture to match the window (post-process,
	// background blur, ...).
	[[nodiscard]] sf::Vector2u GetViewportSize(sf::RenderWindow& window);

	// One directional pass of the "GaussianBlur" shader: sets its "source" and
	// "direction" uniforms, then clears and redraws `output` from `source`.
	// Running this twice -- once with a horizontal direction, once vertical --
	// is a full two-dimensional blur pass; running that pair repeatedly (each
	// iteration reading back the previous output) narrows the blur further.
	// `states` may carry a blend mode or transform; its shader is overwritten
	// with `blurShader`.
	void DrawGaussianBlurPass(sf::RenderTexture& output, const sf::Drawable& source, sf::Shader& blurShader,
		sf::Vector2f direction, const sf::RenderStates& states = {});
}