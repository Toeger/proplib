#include "window.h"
#include "internals/widget.privates.h"
#include "internals/window.privates.h"
#include "widget.h"

#include <SFML/Graphics.hpp>
#include <vector>

static std::vector<prop::Window *> windows;

static auto get_widget_updater(prop::Window &window) {
	return [&window] {
		if (window.widget.get()) {
			window.widget.apply([&window](prop::Polywrap<prop::Widget> &widget) {
				widget->x = widget->y = 0;
				widget->width = {[](int width) { return width; }, window.width};
				widget->height = {[](int height) { return height; }, window.height};
			});
		}
	};
}

prop::Window::Window()
	: Window{Parameters{}} {}

prop::Window::Window(Parameters &&parameters)
	: width{std::move(parameters.width)}
	, height{std::move(parameters.height)}
	, title{std::move(parameters.title)}
	, widget{std::move(parameters.widget)} {
	auto widget_updater = get_widget_updater(*this);
	widget_updater();
	privates.reset(new Window_privates{.window{sf::VideoMode(width.get(), height.get()), title.get()},
									   .on_widget_update{std::move(widget_updater)}});
	windows.push_back(this);
}

prop::Window::~Window() = default;

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
		w.widget.get()->draw({.window = w.privates->window, .offset = {0, 0}});
	}
	window.display();
	return true;
}
