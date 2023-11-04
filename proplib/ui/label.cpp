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

prop::Label::~Label() {}

void prop::Label::update() {
	auto drawer = prop::Widget::privates->window;
	if (!drawer) {
		return;
	}
	sf::Text sftext;
	sftext.setFont(font.get().font_privates->font);
	sftext.setString(text.get());
	sftext.setCharacterSize(24); // in pixels, not points!
	sftext.setFillColor(sf::Color::Black);
	drawer->draw(sftext);
}
