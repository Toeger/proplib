#include "window.h"
#include "../utility/binding.h"
#include "widget.h"
#include "widget.privates.h"

#include <SFML/Graphics.hpp>
#include <vector>

static std::vector<prop::Window *> windows;
namespace prop {
	struct Window_privates {
		bool pump(prop::Window &w);

		sf::RenderWindow window;
		prop::Binding on_widget_update;
	};
} // namespace prop

prop::Window::Window(std::string title, int width, int height)
	: width{width}
	, height{height}
	, title{title}
	, privates{new Window_privates{.window{sf::VideoMode(width, height), title},
								   .on_widget_update{[this] {
									   widget.get();
									   widget.apply([this](std::unique_ptr<prop::Widget> &widget) {
										   if (!widget) {
											   return;
										   }
										   widget->privates->window = &privates->window;
									   });
								   }}}} {
	windows.push_back(this);
}

prop::Window::~Window() {}

void prop::Window::pump() {
	for (auto it = std::begin(windows); it != std::end(windows);) {
		auto &privates = *(*it)->privates;
		if (privates.pump(**it)) {
			++it;
		} else {
			it = windows.erase(it);
		}
	}
}

void prop::Window::exec() {
	while (!windows.empty()) {
		pump();
	}
}

bool prop::Window_privates::pump(prop::Window &w) {
	sf::Event event;
	while (window.pollEvent(event)) {
		if (event.type == sf::Event::Closed) {
			window.close();
			return false;
		}
	}
	window.clear(sf::Color::White);
	if (auto &wp = w.widget.get()) {
		w.widget.get()->update();
	}
	window.display();
	return true;
}
