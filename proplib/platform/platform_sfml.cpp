#include "platform.h"
#include "proplib/ui/window.h"
#include "proplib/utility/canvas.h"
#include "proplib/utility/utility.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>
#include <string>
#include <vector>

struct SFML_window;

static std::vector<SFML_window *> sfml_windows;

namespace prop::platform {
	struct Canvas_context {
		//SFML stuffs
		sf::RenderWindow &window;
	};
} // namespace prop::platform

struct SFML_window : prop::platform::Window {
	SFML_window(int width, int height, std::string_view title, prop::Window *window_)
		: sfml_window{sf::VideoMode(prop::unsigned_cast(width), prop::unsigned_cast(height)), std::string{title}} {
		window = window_;
		sfml_windows.push_back(this);
	}
	SFML_window(const SFML_window &) = delete;
	~SFML_window() {
		std::remove(std::begin(sfml_windows), std::end(sfml_windows), this);
	}
	bool pump(bool exclusive) {
		sf::Event event;
		for (bool success = exclusive ? sfml_window.waitEvent(event) : sfml_window.pollEvent(event); success;
			 success = sfml_window.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				sfml_window.close();
				return false;
			}
		}
		sfml_window.clear(sf::Color::White);
		if (auto &wp = window->widget.get()) {
			prop::platform::Canvas_context canvas_context{sfml_window};
			prop::Canvas canvas{canvas_context, window->size->width, window->size->height};
			window->widget.get()->draw(canvas.sub_canvas_for(*window->widget.get()));
		}
		sfml_window.display();
		return true;
	}
	sf::RenderWindow sfml_window;
};

std::unique_ptr<prop::platform::Window, void (*)(prop::platform::Window *)>
prop::platform::Window::create(prop::platform::Window::Params &&params) {
	return std::unique_ptr<prop::platform::Window, void (*)(prop::platform::Window *)>{
		new SFML_window(params.width, params.height, params.title, params.window),
		[](prop::platform::Window *window_) { delete static_cast<SFML_window *>(window_); }};
}

void prop::platform::Window::pump() {
	for (auto it = std::begin(sfml_windows); it != std::end(sfml_windows);) {
		auto &sfml_window = (**it);
		//TODO: Figure out a way to wait for events from multiple windows
		if (sfml_window.pump(std::size(sfml_windows) == 1)) {
			++it;
		} else {
			it = sfml_windows.erase(it);
		}
	}
}

void prop::platform::Window::exec() {
	while (not sfml_windows.empty()) {
		pump();
	}
}

void prop::platform::canvas::draw_text(Canvas_context &canvas_context, const Rect &rect, std::string_view text,
									   const prop::Font &font) {
	sf::Font sffont;
	if (not sffont.loadFromFile(font.name)) {
		throw prop::Io_error{"Failed opening file " + font.name};
	}
	sf::Text sftext;
	sftext.setPosition(static_cast<float>(rect.left), static_cast<float>(rect.top));
	sftext.setFont(sffont);
	sftext.setString(std::string{text}); //TODO: Figure out a way to avoid the temporary std::string
	sftext.setCharacterSize(static_cast<unsigned int>(font.pixel_size));
	sftext.setFillColor(sf::Color{font.color.r, font.color.g, font.color.b, font.color.a});
	unsigned int style = sf::Text::Regular;
	if (font.italic) {
		style |= sf::Text::Italic;
	}
	if (font.strikeout) {
		style |= sf::Text::StrikeThrough;
	}
	if (font.underline) {
		style |= sf::Text::Underlined;
	}
	if (font.bold) {
		style |= sf::Text::Bold;
	}
	sftext.setStyle(style);
	canvas_context.window.draw(sftext);
}

prop::Size prop::platform::canvas::text_size(std::string_view text, const Font &font) {
	sf::Font sffont;
	if (not sffont.loadFromFile(font.name)) {
		throw prop::Io_error{"Failed opening file \"" + font.name + "\""};
	}
	sf::Text sftext;
	sftext.setFont(sffont);
	sftext.setString(std::string{text}); //TODO: Figure out a way to avoid the temporary std::string
	sftext.setCharacterSize(static_cast<unsigned int>(font.pixel_size));
	unsigned int style = sf::Text::Regular;
	if (font.italic) {
		style |= sf::Text::Italic;
	}
	if (font.strikeout) {
		style |= sf::Text::StrikeThrough;
	}
	if (font.underline) {
		style |= sf::Text::Underlined;
	}
	if (font.bold) {
		style |= sf::Text::Bold;
	}
	sftext.setStyle(style);
	const auto &sfrect = sftext.getLocalBounds();
	return {.width = static_cast<int>(sfrect.width), .height = static_cast<int>(sfrect.height)};
}
