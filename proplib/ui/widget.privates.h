#pragma once

#include <SFML/Graphics/RenderWindow.hpp>

namespace prop {
	struct Widget_privates {
		sf::RenderWindow *window = nullptr;
		sf::Vector2<float> offset{0, 0};
	};
} // namespace prop
