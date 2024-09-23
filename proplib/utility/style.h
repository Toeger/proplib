#pragma once

#include "proplib/utility/font.h"
#include "proplib/utility/property.h"

#include <chrono>

namespace prop {
	class Style {
		public:
		Property<prop::Font> font;
		Property<int> font_size;
		Property<std::chrono::milliseconds> hover_time;
	} extern default_style;
} // namespace prop
