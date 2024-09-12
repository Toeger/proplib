#include "button.h"
#include "internals/button.privates.h"
#include "internals/widget.privates.h"

#include <SFML/Graphics/RectangleShape.hpp>
#include <boost/pfr/tuple_size.hpp>

static void assign(prop::Button &button, prop::Button::Parameters &&parameters) {}

prop::Button::Button()
	: Button{Parameters{}} {}

prop::Button::Button(Parameters &&parameters)
	: prop::Widget{std::move(parameters.widget)}
	, text{std::move(parameters.text)}
	, font{std::move(parameters.font)}
	, font_size{std::move(parameters.font_size)}
	, callback{std::move(parameters.callback)}
	, privates{std::make_unique<Button_privates>(this)} {
	{
		static_assert(boost::pfr::tuple_size_v<prop::Button::Parameters> == 5, "Add missing parameters");
		left_clicked.connect([this] {
			if (callback) {
				callback();
			}
		});
	}
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
	{ //draw frame
		sf::RectangleShape rect{sf::Vector2f(width, height)};
		rect.setPosition(x, y);
		rect.setOutlineColor(sf::Color::Black);
		rect.setOutlineThickness(1);
		rect.setFillColor(sf::Color::Green);
		context.window.draw(rect);
	}

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
}

void prop::swap(Button &lhs, Button &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_X(text)
	PROP_X(font)
	PROP_X(font_size)
	PROP_X(callback)
	PROP_X(privates)
#undef PROP_X
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
