#include "color.h"
#include "prop/platform/platform.h"

prop::Color prop::Color::static_text = prop::Color::silver;
prop::Color prop::Color::type = {{.rgb = 0xB0B000}};
prop::Color prop::Color::type_highlight = {{.rgb = 0xE0E000}};
prop::Color prop::Color::variable_name = {{.rgb = 0x00D000}};
prop::Color prop::Color::variable_value = prop::Color::white;
prop::Color prop::Color::address = prop::Color::purple;
prop::Color prop::Color::address_highlight = {{.rgb = 0xA000A0}};
prop::Color prop::Color::file = prop::Color::aqua;
prop::Color prop::Color::path = prop::Color::teal;
prop::Color prop::Color::function_name = prop::Color::aqua;
prop::Color prop::Color::function_type = prop::Color::teal;

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
