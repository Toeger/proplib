#include "button.h"
#include "proplib/utility/canvas.h"
#include "proplib/utility/utility.h"

#include <boost/pfr/tuple_size.hpp>

prop::Button::Button()
	: Button{Parameters{}} {}

prop::Button::Button(Parameters &&parameters)
	: prop::Widget{std::move(parameters.widget)}
	, text{std::move(parameters.text)}
	, font{std::move(parameters.font)}
	, callback{std::move(parameters.callback)} {
	{
		static_assert(boost::pfr::tuple_size_v<prop::Button::Parameters> == 4, "Add missing parameters");
		left_clicked.connect([this] {
			if (callback) {
				callback();
			}
		});
	}
}

prop::Button::Button(Button &&other) noexcept {
	swap(*this, other);
}

prop::Button &prop::Button::operator=(Button &&other) noexcept {
	swap(*this, other);
	return *this;
}

prop::Button::~Button() = default;

void prop::Button::draw(Canvas canvas) const {
	{	//draw frame
		//sf::RectangleShape rect{sf::Vector2f(width, height)};
		//rect.setPosition(x, y);
		//rect.setOutlineColor(sf::Color::Black);
		//rect.setOutlineThickness(1);
		//rect.setFillColor(sf::Color::Green);
		//canvas.window.draw(rect);
	}

	{ //draw text
		canvas.draw_text(text.get(), font);
		//sf::Text sftext;
		//sftext.setOrigin(canvas.offset);
		//sftext.setPosition(x + 2, y + 2);
		//sftext.setFont(font.get().font_privates->font);
		//sftext.setString(text.get());
		//sftext.setCharacterSize(font_size);
		//sftext.setFillColor(sf::Color::Black);
		//canvas.window.draw(sftext);
	}
}

void prop::swap(Button &lhs, Button &rhs) noexcept {
#define PROP_X(MEMBER) prop::utility::swap(lhs.MEMBER, rhs.MEMBER);
	PROP_X(text)
	PROP_X(font)
	PROP_X(callback)
#undef PROP_X
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
