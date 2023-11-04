#pragma once

#include "../utility/property.h"
#include "font.h"

#include <string>

namespace prop {
	class Style {
		public:
		Property<prop::Font> font;
	} extern default_style;
} // namespace prop
