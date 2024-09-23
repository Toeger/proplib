#include "font.h"
#include "font.privates.h"

#include <SFML/Graphics.hpp>

prop::Font::Font()
	: font_privates{std::make_unique<prop::Font_privates>()} {}

prop::Font::Font(const Font &other)
	: font_privates{std::make_unique<prop::Font_privates>(*other.font_privates)} {}

prop::Font::Font(Font &&other)
	: font_privates{std::move(other.font_privates)} {}

prop::Font &prop::Font::operator=(const prop::Font &other) {
	*font_privates = *other.font_privates;
	return *this;
}

prop::Font &prop::Font::operator=(prop::Font &&other) {
	std::swap(font_privates, other.font_privates);
	return *this;
}

prop::Font::~Font() {}

bool prop::Font::load(std::filesystem::path path) {
	return privates().font.loadFromFile(path.string());
}

prop::Font_privates &prop::Font::privates() {
	return *font_privates;
}

const prop::Font_privates &prop::Font::privates() const {
	return *font_privates;
}
