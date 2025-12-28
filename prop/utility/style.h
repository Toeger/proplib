#pragma once

#include "prop/utility/font.h"
#include "prop/utility/property.h"

#include <chrono>

namespace prop {
	struct Style {
		Property<prop::Font> font;
		Property<std::chrono::milliseconds> hover_time;
		static Style default_style;
	};
} // namespace prop
