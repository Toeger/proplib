#include "label.h"
#include "../utility/font.privates.h"
#include "../utility/style.h"
#include "label.privates.h"
#include "widget.privates.h"

#include <SFML/Graphics.hpp>

prop::Label::Label(std::string text)
	: text{std::move(text)}
	, font{[] { return prop::default_style.font; }}
	, privates{std::make_unique<Label_privates>()} {}

prop::Label::Label(Label &&other) {
	swap(*this, other);
}

prop::Label &prop::Label::operator=(Label &&other) {
	swap(*this, other);
	return *this;
}

prop::Label::~Label() {}

void prop::Label::update() {
	auto drawer = prop::Widget::privates->window;
	if (!drawer) {
		return;
	}

	sf::Text sftext;
	sftext.setOrigin(prop::Widget::privates->offset);
	sftext.setPosition(x, y);
	sftext.setFont(font.get().font_privates->font);
	sftext.setString(text.get());
	sftext.setCharacterSize(24); // in pixels, not points!
	sftext.setFillColor(sf::Color::Black);
	drawer->draw(sftext);
}

void prop::swap(Label &lhs, Label &rhs) {
	using std::swap;
	swap(lhs.text, rhs.text);
	swap(lhs.font, rhs.font);
	swap(lhs.privates, rhs.privates);
	swap(static_cast<Widget &>(lhs), static_cast<Widget &>(rhs));
}
