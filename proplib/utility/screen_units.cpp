#include "screen_units.h"

void prop::reload_screen_dimensions() {
	//TODO: get monitor metrics and fill out values correctly
	prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::millimeters> = 1;
	prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::points> = 1;
	prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_width_percents> = 1;
	prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_height_percents> = 1;
	prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_size_percents> = 1;
}
