#include "screen_units.h"
#include "style.h"

long double prop::detail::get_default_font_size() {
	return static_cast<long double>(prop::Style::default_style.font->size.amount);
}
