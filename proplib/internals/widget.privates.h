#pragma once

#include <SFML/Graphics/RenderWindow.hpp>

namespace prop {
	struct Widget_privates {};

	struct Draw_context {
		sf::RenderWindow &window;
		sf::Vector2<float> offset;
	};
} // namespace prop
