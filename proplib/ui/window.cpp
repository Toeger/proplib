#include "window.h"
#include "../utility/binding.h"
#include "widget.h"
#include "widget.privates.h"

#include <SFML/Graphics.hpp>
#include <vector>

static std::vector<prop::Window *> windows;
namespace prop {
	struct Window_privates {
		bool pump(prop::Window &w, bool exclusive);

		sf::RenderWindow window;
		prop::Binding on_widget_update;
	};
} // namespace prop

static auto get_widget_updater(prop::Window &window) {
	return [&window, widget_privates = static_cast<prop::Widget_privates *>(nullptr)]() mutable {
		auto new_widget_privates = window.widget.get() ? window.widget.get()->privates.get() : nullptr;
		if (new_widget_privates == widget_privates) {
			return;
		}
		if (widget_privates) {
			widget_privates->window = nullptr;
		}
		widget_privates = new_widget_privates;
		if (widget_privates) {
			widget_privates->window = &window.privates->window;
			widget_privates->offset = {0, 0};
			window.widget.apply(
				[window_privates = window.privates.get(), &window](prop::Polywrap<prop::Widget> &widget) {
					widget->privates->window = &window_privates->window;
					widget->x = 0;
					widget->y = 0;
					widget->width.bind(window.width);
					widget->height.bind(window.height);
				});
		}
	};
}

prop::Window::Window(std::string title, int width, int height)
	: width{width}
	, height{height}
	, title{title}
	, privates{new Window_privates{.window{sf::VideoMode(width, height), title},
								   .on_widget_update{get_widget_updater(*this)}}} {
	windows.push_back(this);
}

prop::Window::~Window() {}

void prop::Window::pump() {
	for (auto it = std::begin(windows); it != std::end(windows);) {
		auto &window = (**it);
		//TODO: Figure out a way to wait for events from multiple windows
		if (window.privates->pump(window, std::size(windows) == 1)) {
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

bool prop::Window_privates::pump(prop::Window &w, bool exclusive) {
	sf::Event event;
	for (bool success = exclusive ? window.waitEvent(event) : window.pollEvent(event); success;
		 success = window.pollEvent(event)) {
		if (event.type == sf::Event::Closed) {
			window.close();
			return false;
		}
	}
	window.clear(sf::Color::White);
	if (auto &wp = w.widget.get()) {
		w.widget.apply([](prop::Polywrap<Widget> &w) { w->update(); });
	}
	window.display();
	return true;
}
