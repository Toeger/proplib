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
				widget->width = {+[](int width) { return width; }, window.width};
				widget->height = {+[](int height) { return height; }, window.height};
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

prop::Window::Window(std::string title, prop::Polywrap<Widget> widget, int width, int height)
	: Window{std::move(title), width, height} {
	this->widget = std::move(widget);
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
