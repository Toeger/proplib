/**
 * MIT License
 *
 * Copyright (c) 2024 NoEvaa
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef AYIN_ANSI_COLOR_HPP_
#define AYIN_ANSI_COLOR_HPP_

#include <cassert>
#include <charconv>
#include <cstdint>
#include <ostream>

namespace ansi_color {
	enum class ColorSpec {
		kNone = 0,
		kReset,
		kDefault,
		kRgb,

		kBlack = 10,
		kRed,
		kGreen,
		kBrown,
		kBlue,
		kMagenta,
		kCyan,
		kWhite,
	};

	enum class ColorTarget {
		kForeground = 0,
		kBackground,

		kFg = kForeground,
		kBg = kBackground,
	};

	namespace detail {
		struct RgbColor {
			std::uint8_t red_ = 0;
			std::uint8_t green_ = 0;
			std::uint8_t blue_ = 0;

			constexpr RgbColor() = default;
			constexpr RgbColor(int _r, int _g, int _b)
				: red_(_r & 0xff)
				, green_(_g & 0xff)
				, blue_(_b & 0xff) {}
		};

		struct Color {
			ColorSpec spec_ = ColorSpec::kReset;
			RgbColor rgb_;

			constexpr Color() = default;
			constexpr Color(ColorSpec _spec)
				: spec_(_spec) {}
			constexpr Color(int _r, int _g, int _b)
				: spec_(ColorSpec::kRgb)
				, rgb_(_r, _g, _b) {}
		};

		/**
		 * See `https://www.man7.org/linux/man-pages/man4/console_codes.4.html`
		 * for detail of console codes.
		 */
		inline int _toConsoleCode(ColorSpec _cs) {
			switch (_cs) {
				case ColorSpec::kBlack:
					return 0;
				case ColorSpec::kRed:
					return 1;
				case ColorSpec::kGreen:
					return 2;
				case ColorSpec::kBrown:
					return 3;
				case ColorSpec::kBlue:
					return 4;
				case ColorSpec::kMagenta:
					return 5;
				case ColorSpec::kCyan:
					return 6;
				case ColorSpec::kWhite:
					return 7;
				case ColorSpec::kRgb:
					return 8;
				case ColorSpec::kDefault:
					return 9;
				default:
					break;
			}
			assert(0); // unreachable
			return -1;
		}
	} // namespace detail

	struct AnsiColor {
		ColorTarget target_ = ColorTarget::kFg;
		detail::Color color_;

		constexpr AnsiColor() = default;
		constexpr AnsiColor(detail::Color const &_c, ColorTarget _tgt = ColorTarget::kFg)
			: target_(_tgt)
			, color_(_c) {}
	};

	inline std::ostream &operator<<(std::ostream &ost, AnsiColor const &ac) {
		switch (ac.color_.spec_) {
			case ColorSpec::kNone:
				return ost;
			case ColorSpec::kReset:
				return (ost << "\033[0m");
			default:
				break;
		}

		// "\033[00;2;000;000;000m"
		constexpr std::size_t kBufSize = 4 +		 // \033[00
										 3 +		 // ;2;
										 3 * 3 + 2 + // 000;000;000
										 1;			 // m
		char buf[kBufSize];

		char *p = buf;
		*p++ = '\033';
		*p++ = '[';

		// foreground: 30-39
		// background: 40-49
		int co = detail::_toConsoleCode(ac.color_.spec_);
		int co_offset = (ac.target_ == ColorTarget::kBg) ? 40 : 30;
		p = std::to_chars(p, p + 2, co + co_offset).ptr;

		// rgb mode
		if (co == 8) {
			const auto &rgb = ac.color_.rgb_;
			*p++ = ';';
			*p++ = '2';
			*p++ = ';';
			p = std::to_chars(p, p + 3, rgb.red_).ptr;
			*p++ = ';';
			p = std::to_chars(p, p + 3, rgb.green_).ptr;
			*p++ = ';';
			p = std::to_chars(p, p + 3, rgb.blue_).ptr;
		}

		*p++ = 'm';

		ost.write(buf, static_cast<std::streamsize>(p - buf));
		return ost;
	}
} // namespace ansi_color
#endif
