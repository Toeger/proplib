#include "label.h"
#include "../internals/label.privates.h"
#include "../internals/widget.privates.h"
#include "../utility/font.privates.h"
#include "../utility/style.h"

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

prop::Label::~Label() = default;

void prop::Label::draw(Draw_context context) const {
	sf::Text sftext;
	sftext.setOrigin(context.offset);
	sftext.setPosition(x, y);
	sftext.setFont(font.get().font_privates->font);
	sftext.setString(text.get());
	sftext.setCharacterSize(font_size);
	sftext.setFillColor(sf::Color::Black);
	context.window.draw(sftext);
}

void prop::swap(Label &lhs, Label &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_X(text)
	PROP_X(font)
	PROP_X(font_size)
	PROP_X(privates)
#undef PROP_X
		swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
