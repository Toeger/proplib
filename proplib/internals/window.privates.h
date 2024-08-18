#pragma once

#include <SFML/Graphics/RenderWindow.hpp>

#include "ui/window.h"

namespace prop {
	struct Window_privates {
		bool pump(prop::Window &w, bool exclusive);

		sf::RenderWindow window;
		prop::Property<void> on_widget_update;
	};
} // namespace prop
