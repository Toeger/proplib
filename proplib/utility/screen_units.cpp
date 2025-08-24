#include "screen_units.h"
#include "proplib/platform/platform.h"

void prop::reload_screen_dimensions() {
	const auto screens = prop::platform::get_screens();
	if (screens.empty()) {
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::x_millimeters> = 1;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::y_millimeters> = 1;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::points> = 1;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_width_percents> = 1;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_height_percents> = 1;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_size_percents> = 1;
	} else {
		//TODO: Select screen properly
		auto &screen = screens[0];
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::x_millimeters> =
			screen.width_pixels / screen.width_mm;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::y_millimeters> =
			screen.height_pixels / screen.height_mm;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::points> =
			screen.height_pixels * 25.4l / 72 / screen.height_mm;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_width_percents> = screen.width_pixels / 100;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_height_percents> = screen.height_pixels / 100;
		prop::screen_unit_to_pixels_factor<prop::Screen_unit_type::screen_size_percents> =
			screen.width_pixels * screen.height_pixels / 100 / 100;
	}
}
