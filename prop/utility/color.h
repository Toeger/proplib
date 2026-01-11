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
		static Color static_text, type, type_highlight, variable_name, variable_value, address, address_highlight, file,
			path, function_name, function_type;

		struct Reset {
		} static constexpr reset{};
	};

	struct Background {
		prop::Color color = Color::black;
	};

	std::ostream &operator<<(std::ostream &os, prop::Color color);
	std::ostream &operator<<(std::ostream &os, prop::Background background);
	std::ostream &operator<<(std::ostream &os, prop::Color::Reset);

} // namespace prop

template <>
struct std::formatter<prop::Color, char> {
	enum class Color_format { ansi, hex } color_format;

	template <class ParseContext>
	constexpr ParseContext::iterator parse(ParseContext &ctx) {
		std::string arg;
		auto it = ctx.begin();
		for (; it != ctx.end() && *it != '}'; ++it) {
			arg.push_back(*it);
		}
		if (arg == "ansi") {
			color_format = Color_format::ansi;
		} else if (arg == "hex") {
			color_format = Color_format::hex;
		} else {
			throw std::format_error{"Invalid format specifier \"" + arg + "\" for prop::Color"};
		}
		return it;
	}

	template <class FmtContext>
	FmtContext::iterator format(const prop::Color &color, FmtContext &ctx) const {
		switch (color_format) {
			case Color_format::ansi:
				return std::format_to(ctx.out(), "\033[38;2;{};{};{}m", color.r, color.g, color.b);
			case Color_format::hex:
				return std::format_to(ctx.out(), "{:02X}{:02X}{:02X}", color.r, color.g, color.b);
		}
		return ctx.out();
	}
};

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
inline prop::Color prop::Color::type = prop::Color::olive;
inline prop::Color prop::Color::type_highlight = prop::Color::yellow;
inline prop::Color prop::Color::variable_name = prop::Color::lime;
inline prop::Color prop::Color::variable_value = prop::Color::white;
inline prop::Color prop::Color::address = prop::Color::purple;
inline prop::Color prop::Color::address_highlight = prop::Color::fuchsia;
inline prop::Color prop::Color::file = prop::Color::aqua;
inline prop::Color prop::Color::path = prop::Color::teal;
inline prop::Color prop::Color::function_name = prop::Color::aqua;
inline prop::Color prop::Color::function_type = prop::Color::teal;
