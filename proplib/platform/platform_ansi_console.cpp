#include "external/ansi_color.hpp"
#include "platform.h"
#include "proplib/utility/color.h"
#include <iostream>

#ifdef WIN32
//Windows console ANSI mode has to be enabled before it can be used. The easiest way to do this is with system("");
//This is probably a bug in Windows, but it will probably never be fixed because it's too useful.

#include <cstdlib>

static const int enable_windows_ansi_mode = std::system("");
#endif //WIN32

void prop::platform::Console::set_foreground_text_color(std::ostream &os, prop::Color color) {
	os << ansi_color::AnsiColor{{color.r, color.g, color.b}, ansi_color::ColorTarget::kForeground};
}
void prop::platform::Console::set_background_text_color(std::ostream &os, prop::Color color) {
	os << ansi_color::AnsiColor{{color.r, color.g, color.b}, ansi_color::ColorTarget::kBackground};
}
void prop::platform::Console::reset_text_color(std::ostream &os) {
	os << ansi_color::AnsiColor{{}, ansi_color::ColorTarget::kForeground}
	   << ansi_color::AnsiColor{{}, ansi_color::ColorTarget::kBackground};
}
