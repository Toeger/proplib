#include "button.h"
#include "internals/button.privates.h"
#include "internals/widget.privates.h"
#include "utility/style.h"

#include <SFML/Graphics/RectangleShape.hpp>

prop::Button::Button()
	: font{[] { return prop::default_style.font; }}
	, font_size{[] { return prop::default_style.font_size; }}
	, privates{std::make_unique<Button_privates>(this)} {
	{
		left_clicked.connect([this] {
			if (on_clicked) {
				on_clicked();
			}
		});
	}
}

prop::Button::Button(std::string text, std::function<void ()> on_clicked)
	: Button() {
	this->text = std::move(text);
	this->on_clicked = std::move(on_clicked);
}

prop::Button::Button(Button &&other) {
	swap(*this, other);
}

prop::Button &prop::Button::operator=(Button &&other) {
	swap(*this, other);
	return *this;
}

prop::Button::~Button() = default;

void prop::Button::draw(Draw_context context) const {
	{ //draw text
		sf::Text sftext;
		sftext.setOrigin(context.offset);
		sftext.setPosition(x + 2, y + 2);
		sftext.setFont(font.get().font_privates->font);
		sftext.setString(text.get());
		sftext.setCharacterSize(font_size);
		sftext.setFillColor(sf::Color::Black);
		context.window.draw(sftext);
	}

	{ //draw frame
		sf::RectangleShape rect{sf::Vector2f(width, height)};
		rect.setPosition(x, y);
		rect.setOutlineColor(sf::Color::Black);
		rect.setOutlineThickness(1);
		rect.setFillColor(sf::Color::Green);
		context.window.draw(rect);
	}
}

void prop::swap(Button &lhs, Button &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_X(text)
	PROP_X(font)
	PROP_X(font_size)
	PROP_X(on_clicked)
	PROP_X(privates)
#undef PROP_X
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
