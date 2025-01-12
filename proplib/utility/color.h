#pragma once

#include <cstdint>
#include <ostream>

namespace prop {
	class Color {
		struct Color_r_g_b_a {
			std::uint8_t r = 0;
			std::uint8_t g = 0;
			std::uint8_t b = 0;
			std::uint8_t a = 255;
		};

		struct Color_rgba {
			std::uint32_t rgba;
		};
		struct Color_argb {
			std::uint32_t argb;
		};
		struct Color_rgb {
			std::uint32_t rgb;
		};

		public:
		constexpr Color(Color_r_g_b_a color_r_g_b_a)
			: r{color_r_g_b_a.r}
			, g{color_r_g_b_a.g}
			, b{color_r_g_b_a.b}
			, a{color_r_g_b_a.a} {}

		constexpr Color(Color_rgba color_rgba)
			: r(color_rgba.rgba >> 24 & 0xff)
			, g(color_rgba.rgba >> 16 & 0xff)
			, b(color_rgba.rgba >> 8 & 0xff)
			, a(color_rgba.rgba >> 0 & 0xff) {}

		constexpr Color(Color_argb color_argb)
			: r(color_argb.argb >> 16 & 0xff)
			, g(color_argb.argb >> 8 & 0xff)
			, b(color_argb.argb >> 0 & 0xff)
			, a(color_argb.argb >> 24 & 0xff) {}

		constexpr Color(Color_rgb color_rgb)
			: r(color_rgb.rgb >> 16 & 0xff)
			, g(color_rgb.rgb >> 8 & 0xff)
			, b(color_rgb.rgb >> 0 & 0xff)
			, a(255) {}

		constexpr Color(const Color &other) = default;
		constexpr Color &operator=(const Color &other) = default;

		std::uint8_t r{};
		std::uint8_t g{};
		std::uint8_t b{};
		std::uint8_t a{};

		//web colors
		static const Color white, silver, gray, black, red, maroon, yellow, olive, lime, green, aqua, teal, blue, navy,
			fuchsia, purple;

		//prop colors
		static Color static_text, type, pointer, file, path, function_name, function_type;
	};

	struct Console_text_color {
		prop::Color color = Color::black;
		friend std::ostream &operator<<(std::ostream &os, Console_text_color cc);
	};
	std::string to_string(Console_text_color ctc);
	struct Console_background_text_color {
		prop::Color color = Color::black;
		friend std::ostream &operator<<(std::ostream &os, Console_background_text_color cc);
	};
	std::string to_string(Console_background_text_color cbtc);
	struct Console_reset_text_color {
		friend std::ostream &operator<<(std::ostream &os, Console_reset_text_color cc);
	} constexpr console_reset_text_color;
	std::string to_string(Console_reset_text_color crtc);
} // namespace prop

inline constexpr prop::Color prop::Color::white{{.rgb = 0xFFFFFF}};
inline constexpr prop::Color prop::Color::silver{{.rgb = 0xC0C0C0}};
inline constexpr prop::Color prop::Color::gray{{.rgb = 0x808080}};
inline constexpr prop::Color prop::Color::black{{.rgb = 0x000000}};
inline constexpr prop::Color prop::Color::red{{.rgb = 0xFF0000}};
inline constexpr prop::Color prop::Color::maroon{{.rgb = 0x800000}};
inline constexpr prop::Color prop::Color::yellow{{.rgb = 0xFFFF00}};
inline constexpr prop::Color prop::Color::olive{{.rgb = 0x808000}};
inline constexpr prop::Color prop::Color::lime{{.rgb = 0x00FF00}};
inline constexpr prop::Color prop::Color::green{{.rgb = 0x008000}};
inline constexpr prop::Color prop::Color::aqua{{.rgb = 0x00FFFF}};
inline constexpr prop::Color prop::Color::teal{{.rgb = 0x008080}};
inline constexpr prop::Color prop::Color::blue{{.rgb = 0x0000FF}};
inline constexpr prop::Color prop::Color::navy{{.rgb = 0x000080}};
inline constexpr prop::Color prop::Color::fuchsia{{.rgb = 0xFF00FF}};
inline constexpr prop::Color prop::Color::purple{{.rgb = 0x800080}};

inline prop::Color prop::Color::static_text = prop::Color::silver;
inline prop::Color prop::Color::type = prop::Color::fuchsia;
inline prop::Color prop::Color::pointer = prop::Color::purple;
inline prop::Color prop::Color::file = prop::Color::aqua;
inline prop::Color prop::Color::path = prop::Color::teal;
inline prop::Color prop::Color::function_name = prop::Color::aqua;
inline prop::Color prop::Color::function_type = prop::Color::teal;
