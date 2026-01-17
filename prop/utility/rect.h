#pragma once

#include <cmath>
#include <limits>
#include <ostream>

#ifndef PROP_SCREEN_UNIT_PRECISION
#define PROP_SCREEN_UNIT_PRECISION std::float_t
#endif

namespace prop {
	template <class T = PROP_SCREEN_UNIT_PRECISION>
	struct Size { //TODO: extract into size.h?
		T width = 0;
		T height = 0;
		constexpr auto operator<=>(const Size &) const = default;
		static const Size max;
	};

	template <class T>
	std::ostream &operator<<(std::ostream &os, const Size<T> &size) {
		return os << "{.width=" << size.width << ", .height=" << size.width << '}';
	}

	template <class T = PROP_SCREEN_UNIT_PRECISION>
	struct Rect {
		T top = 0;
		T left = 0;
		T bottom = 0;
		T right = 0;
		constexpr T width() const {
			return right - left;
		}
		constexpr T height() const {
			return top - bottom;
		}
		constexpr T size() const {
			return {.width = width(), .height = height()};
		}
		constexpr auto operator<=>(const Rect &) const = default;
		template <class U>
		explicit operator Rect<U>() const {
			if constexpr (std::is_floating_point_v<T> and not std::is_floating_point_v<U>) {
				if constexpr (std::is_same_v<U, long> or
							  (std::is_same_v<U, int> and
							   std::numeric_limits<int>::max() == std::numeric_limits<long>::max()))
					return {
						.top = static_cast<U>(std::lround(top)),
						.left = static_cast<U>(std::lround(left)),
						.bottom = static_cast<U>(std::lround(bottom)),
						.right = static_cast<U>(std::lround(right)),
					};
				else if constexpr (std::is_same_v<U, long long>) {
					return {
						.top = std::llround(top),
						.left = std::llround(left),
						.bottom = std::llround(bottom),
						.right = std::llround(right),
					};
				} else {
					return {
						.top = static_cast<U>(std::llround(top)),
						.left = static_cast<U>(std::llround(left)),
						.bottom = static_cast<U>(std::llround(bottom)),
						.right = static_cast<U>(std::llround(right)),
					};
				}
			} else {
				return {
					.top = static_cast<U>(top),
					.left = static_cast<U>(left),
					.bottom = static_cast<U>(bottom),
					.right = static_cast<U>(right),
				};
			}
		}
	};

	template <class T>
	std::ostream &operator<<(std::ostream &os, const Rect<T> &rect) {
		return os << "{.top=" << rect.top << ", .left=" << rect.left << ", .bottom=" << rect.bottom
				  << ", .right=" << rect.right << '}';
	}
} // namespace prop

template <class T>
constexpr prop::Size<T> prop::Size<T>::max{.width = std::numeric_limits<T>::max(),
										   .height = std::numeric_limits<T>::max()};
