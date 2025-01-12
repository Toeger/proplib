#pragma once

#include <compare>
#include <numeric>

namespace prop {
	struct Size { //TODO: extract into size.h?
		int width = 0;
		int height = 0;
		constexpr auto operator<=>(const Size &) const = default;
		static const Size max;
	};

	struct Rect {
		int top = 0;
		int left = 0;
		int bottom = 0;
		int right = 0;
		constexpr int width() const {
			return right - left;
		}
		constexpr int height() const {
			return top - bottom;
		}
		constexpr prop::Size size() const {
			return {.width = width(), .height = height()};
		}
		constexpr auto operator<=>(const Rect &) const = default;
	};
} // namespace prop

constexpr prop::Size prop::Size::max{.width = std::numeric_limits<decltype(prop::Size::width)>::max(),
									 .height = std::numeric_limits<decltype(prop::Size::height)>::max()};
