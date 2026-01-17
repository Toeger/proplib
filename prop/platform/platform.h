#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "prop/utility/rect.h"

namespace prop {
	class Window;
	class Font;
	class Color;

	namespace platform {
		class Window {
			struct Params {
				int width = 800;
				int height = 600;
				std::string_view title = "";
				prop::Window *window;
			};

			protected:
			Window() = default;
			Window(const Window &) = delete;
			~Window() = default;

			public:
			void set_title(std::string_view title);
			void set_size(int width, int height);

			static std::unique_ptr<Window, void (*)(prop::platform::Window *)> create(Params &&params);
			static void pump();
			static void exec();

			prop::Window *window;
		};

		struct Canvas_context;
		namespace canvas {
			void draw_text(Canvas_context &canvas_context, const prop::Rect<> &rect, std::string_view text,
						   const Font &font);
			void draw_rect(Canvas_context &canvas_context, prop::Rect<> rect, prop::Color color, float width_px);
			int text_height(const prop::Font &font);
			prop::Size<> text_size(std::string_view text, const Font &font);

		} // namespace canvas

		namespace Console {
			void set_foreground_text_color(std::ostream &os, prop::Color color);
			void set_background_text_color(std::ostream &os, prop::Color color);
			void reset_text_color(std::ostream &os);
		} // namespace Console

		struct Screen {
			long double width_pixels;
			long double height_pixels;
			long double x_origin_pixels;
			long double y_origin_pixels;
			long double x_dpi;
			long double y_dpi;
			auto operator<=>(const Screen &) const = default;
		};
		enum class Get_screens_strategy {
			compatibility, //mimic whatever the platform does, even if incorrect
			correctness,   //try to obtain the correct values
			fixed_96_DPI,  //ignore platform DPI
		};
		std::vector<Screen>
		get_screens(Get_screens_strategy get_screens_strategy = Get_screens_strategy::compatibility);

		//TODO: font dimensions
	} // namespace platform
} // namespace prop
