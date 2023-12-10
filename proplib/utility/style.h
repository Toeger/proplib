#pragma once

#include "../utility/property.h"
#include "font.h"

#include <chrono>
#include <string>

namespace prop {
	class Style {
		public:
		Property<prop::Font> font;
		Property<int> font_size;
		Property<std::chrono::milliseconds> hover_time;
	} extern default_style;
} // namespace prop
