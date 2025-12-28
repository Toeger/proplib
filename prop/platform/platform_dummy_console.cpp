#include "platform.h"
#include "proplib/utility/color.h"

void prop::platform::Console::set_foreground_text_color(std::ostream &, prop::Color) {}
void prop::platform::Console::set_background_text_color(std::ostream &, prop::Color) {}
void prop::platform::Console::reset_text_color(std::ostream &) {}
