#include "color.h"
#include "proplib/platform/platform.h"

std::ostream &prop::operator<<(std::ostream &os, prop::Color color) {
	prop::platform::Console::set_foreground_text_color(os, color);
	return os;
}

std::ostream &prop::operator<<(std::ostream &os, prop::Background background) {
	prop::platform::Console::set_background_text_color(os, background.color);
	return os;
}

std::ostream &prop::operator<<(std::ostream &os, prop::Color::Reset) {
	prop::platform::Console::reset_text_color(os);
	return os;
}
