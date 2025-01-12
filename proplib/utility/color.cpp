#include "color.h"
#include "proplib/platform/platform.h"
#include <sstream>

std::ostream &prop::operator<<(std::ostream &os, prop::Console_text_color cc) {
	prop::platform::Console::set_foreground_text_color(os, cc.color);
	return os;
}

std::ostream &prop::operator<<(std::ostream &os, prop::Console_background_text_color cc) {
	prop::platform::Console::set_background_text_color(os, cc.color);
	return os;
}

std::ostream &prop::operator<<(std::ostream &os, prop::Console_reset_text_color) {
	prop::platform::Console::reset_text_color(os);
	return os;
}

std::string prop::to_string(Console_text_color ctc) {
	std::stringstream ss;
	ss << ctc;
	return std::move(ss).str();
}

std::string prop::to_string(Console_background_text_color cbtc) {
	std::stringstream ss;
	ss << cbtc;
	return std::move(ss).str();
}

std::string prop::to_string(Console_reset_text_color crtc) {
	std::stringstream ss;
	ss << crtc;
	return std::move(ss).str();
}
