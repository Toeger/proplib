#pragma once

#include "proplib/utility/font.h"
#include "proplib/utility/rect.h"

#include <string_view>

namespace prop {
	class Widget;
	class Window;
	namespace platform {
		struct Canvas_context;
	}

	class Canvas {
		Rect rect;
		Canvas(Rect rect_, prop::platform::Canvas_context &canvas_context_);

		public:
		Canvas(prop::platform::Canvas_context &canvas_context_, int width_, int height_);
		void draw_text(std::string_view text);
		void draw_text(std::string_view text, prop::Font font_);
		Canvas sub_canvas_for(const prop::Widget &widget);
		prop::Font font;

		private:
		prop::platform::Canvas_context *canvas_context;
	};
} // namespace prop
