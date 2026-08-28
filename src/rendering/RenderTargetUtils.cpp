#include "RenderTargetUtils.h"

#include <algorithm>

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shader.hpp>

namespace Rendering
{
	sf::Vector2u GetViewportSize(sf::RenderWindow& window)
	{
		const sf::IntRect viewport{ window.getViewport(window.getView()) };
		if (viewport.size.x <= 0 || viewport.size.y <= 0)
		{
			const sf::Vector2u windowSize{ window.getSize() };

			return 
			{ 
				std::max(1u, windowSize.x), 
				std::max(1u, windowSize.y)
			};
		}
		else
		{
			return
			{
				static_cast<unsigned int>(viewport.size.x),
				static_cast<unsigned int>(viewport.size.y)
			};
		}
	}

	void DrawGaussianBlurPass(sf::RenderTexture& output, const sf::Drawable& source, sf::Shader& blurShader,
		sf::Vector2f direction, const sf::RenderStates& states)
	{
		blurShader.setUniform("source", sf::Shader::CurrentTexture);
		blurShader.setUniform("direction", sf::Glsl::Vec2(direction.x, direction.y));

		sf::RenderStates blurStates{ states };
		blurStates.shader = &blurShader;

		output.clear(sf::Color::Transparent);
		output.draw(source, blurStates);
		output.display();
	}
}