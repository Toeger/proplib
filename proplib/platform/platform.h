#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace prop {
	class Window;
	class Font;
	struct Rect;
	struct Size;
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
			void draw_text(Canvas_context &canvas_context, const Rect &rect, std::string_view text, const Font &font);
			int text_height(const prop::Font &font);
			prop::Size text_size(std::string_view text, const Font &font);

		} // namespace canvas

		namespace Console {
			void set_foreground_text_color(std::ostream &os, prop::Color color);
			void set_background_text_color(std::ostream &os, prop::Color color);
			void reset_text_color(std::ostream &os);
		} // namespace Console

		struct Screen {
			long double width_pixels;
			long double height_pixels;
			long double width_mm;
			long double height_mm;
			long double x_origin_pixels;
			long double y_origin_pixels;
		};
		std::vector<Screen> get_screens();

		//TODO: font dimensions
	} // namespace platform
} // namespace prop
