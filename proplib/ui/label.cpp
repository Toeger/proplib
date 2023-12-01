#include "label.h"
#include "../utility/font.privates.h"
#include "../utility/style.h"
#include "label.privates.h"
#include "widget.privates.h"

#include <SFML/Graphics.hpp>

prop::Label::Label(std::string text)
	: text{std::move(text)}
	, font{[] { return prop::default_style.font; }}
	, font_size{[] { return prop::default_style.font_size; }} {
	privates = std::make_unique<Label_privates>(this);
}

prop::Label::Label(Label &&other) {
	swap(*this, other);
}

prop::Label &prop::Label::operator=(Label &&other) {
	swap(*this, other);
	privates->label = this;
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
	sftext.setCharacterSize(font_size);
	sftext.setFillColor(sf::Color::Black);
	drawer->draw(sftext);
}

void prop::swap(prop::Label &lhs, prop::Label &rhs) {
	using std::swap;
#define PROP_MEMBERS PROP_X(text) PROP_X(font) PROP_X(font_size) PROP_X(privates)
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_MEMBERS
#undef PROP_X
#undef PROP_MEMBERS
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
