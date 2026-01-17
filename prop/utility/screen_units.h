#pragma once

#include "prop/platform/platform.h"
#include "prop/utility/property.h"

#include <cmath>
#include <format>
#include <iterator>
#include <ostream>
#include <type_traits>

#if __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

namespace prop {
#ifndef PROP_SCREEN_UNIT_PRECISION
#define PROP_SCREEN_UNIT_PRECISION std::float_t
#endif
	using Screen_unit_precision = PROP_SCREEN_UNIT_PRECISION;
	static_assert(std::is_floating_point_v<Screen_unit_precision>,
				  "Floating point type required for PROP_SCREEN_UNIT_PRECISION");

	enum class Screen_unit_type {
		pixels,
		x_millimeters,
		y_millimeters,
		points,
		text_sizes,
		screen_width_percents,
		screen_height_percents,
		screen_size_percents,
	};
	inline prop::Property<prop::platform::Get_screens_strategy> get_screens_strategy =
		prop::platform::Get_screens_strategy::compatibility;
	const inline prop::Property<std::vector<prop::platform::Screen>> screens = [] {
		return prop::platform::get_screens(get_screens_strategy);
	};

	namespace detail {
		long double get_default_font_size();
	}

	template <Screen_unit_type screen_unit_type>
	const inline prop::Property<long double> screen_unit_to_pixels_factor = [] -> long double {
		if (screens->empty()) {
			return 0;
		}
		//TODO: Do better at picking the correct screen
		auto &window_screen = screens[0u];
		if constexpr (screen_unit_type == Screen_unit_type::x_millimeters) {
			return window_screen.x_dpi / 25.4;
		} else if constexpr (screen_unit_type == Screen_unit_type::y_millimeters) {
			return window_screen.y_dpi / 25.4;
		} else if constexpr (screen_unit_type == Screen_unit_type::points) {
			return window_screen.y_dpi / 72;
		} else if constexpr (screen_unit_type == Screen_unit_type::text_sizes) {
			return prop::detail::get_default_font_size();
		} else if constexpr (screen_unit_type == Screen_unit_type::screen_width_percents) {
			return window_screen.width_pixels / 100;
		} else if constexpr (screen_unit_type == Screen_unit_type::screen_height_percents) {
			return window_screen.height_pixels / 100;
		} else if constexpr (screen_unit_type == Screen_unit_type::screen_size_percents) {
			return window_screen.width_pixels * window_screen.height_pixels / 100 / 100;
		} else {
			static_assert(false, "Add missing screen unit type");
		}
	};

	template <>
	constexpr inline long double screen_unit_to_pixels_factor<Screen_unit_type::pixels> = 1;

	template <Screen_unit_type screen_unit_type>
	struct Screen_unit {
		Screen_unit_precision amount{};
		explicit constexpr Screen_unit(Screen_unit_precision amount_ = 0) noexcept
			: amount{amount_} {}
		template <Screen_unit_type other_screen_unit_type>
		constexpr Screen_unit(Screen_unit<other_screen_unit_type> other_unit) noexcept {
			*this = other_unit;
		}
		template <Screen_unit_type other_screen_unit_type>
		constexpr Screen_unit &operator=(Screen_unit<other_screen_unit_type> other_unit) noexcept {
			if constexpr (screen_unit_type == other_screen_unit_type) {
				amount = other_unit.amount;
			} else {
				amount = static_cast<Screen_unit_precision>(
					static_cast<decltype(screen_unit_to_pixels_factor<screen_unit_type>)>(other_unit.amount) *
					screen_unit_to_pixels_factor<other_screen_unit_type> /
					screen_unit_to_pixels_factor<screen_unit_type>);
			}
			return *this;
		}
	};

	template <Screen_unit_type screen_unit_type_lhs, Screen_unit_type screen_unit_type_rhs>
	constexpr Screen_unit<screen_unit_type_lhs> &operator+=(Screen_unit<screen_unit_type_lhs> &lhs,
															Screen_unit<screen_unit_type_rhs> rhs) noexcept {
		if constexpr (screen_unit_type_lhs == screen_unit_type_rhs) {
			lhs.amount += rhs.amount;
		} else {
			lhs.amount += rhs.amount * screen_unit_to_pixels_factor<screen_unit_type_rhs> /
						  screen_unit_to_pixels_factor<screen_unit_type_lhs>;
		}
		return lhs;
	}

	template <Screen_unit_type screen_unit_type_lhs, Screen_unit_type screen_unit_type_rhs>
	constexpr Screen_unit<screen_unit_type_lhs> &operator-=(Screen_unit<screen_unit_type_lhs> &lhs,
															Screen_unit<screen_unit_type_rhs> rhs) noexcept {
		if constexpr (screen_unit_type_lhs == screen_unit_type_rhs) {
			lhs.amount -= rhs.amount;
		} else {
			lhs.amount -= rhs.amount * screen_unit_to_pixels_factor<screen_unit_type_rhs> /
						  screen_unit_to_pixels_factor<screen_unit_type_lhs>;
		}
		return lhs;
	}

	template <Screen_unit_type screen_unit_type>
	constexpr Screen_unit<screen_unit_type> &operator*=(Screen_unit<screen_unit_type> &lhs,
														Screen_unit_precision scalar) noexcept {
		lhs.amount *= scalar;
		return lhs;
	}

	template <Screen_unit_type screen_unit_type>
	constexpr Screen_unit<screen_unit_type> &operator/=(Screen_unit<screen_unit_type> &lhs,
														Screen_unit_precision scalar) noexcept {
		lhs.amount /= scalar;
		return lhs;
	}

	template <Screen_unit_type screen_unit_type_lhs, Screen_unit_type screen_unit_type_rhs>
	[[nodiscard]] constexpr auto operator+(Screen_unit<screen_unit_type_lhs> lhs,
										   Screen_unit<screen_unit_type_rhs> rhs) noexcept {
		return lhs += rhs;
	}

	template <Screen_unit_type screen_unit_type_lhs, Screen_unit_type screen_unit_type_rhs>
	[[nodiscard]] constexpr auto operator-(Screen_unit<screen_unit_type_lhs> lhs,
										   Screen_unit<screen_unit_type_rhs> rhs) noexcept {
		return lhs -= rhs;
	}

	template <Screen_unit_type screen_unit_type>
	[[nodiscard]] constexpr Screen_unit<screen_unit_type> operator*(Screen_unit<screen_unit_type> lhs,
																	Screen_unit_precision scalar) noexcept {
		return lhs *= scalar;
	}

	template <Screen_unit_type screen_unit_type>
	[[nodiscard]] constexpr Screen_unit<screen_unit_type> operator/(Screen_unit<screen_unit_type> lhs,
																	Screen_unit_precision scalar) noexcept {
		return lhs /= scalar;
	}

	template <Screen_unit_type screen_unit_type_lhs, Screen_unit_type screen_unit_type_rhs>
	[[nodiscard]] constexpr Screen_unit_precision operator/(Screen_unit<screen_unit_type_lhs> lhs,
															Screen_unit<screen_unit_type_rhs> rhs) noexcept {
		if constexpr (screen_unit_type_lhs == screen_unit_type_rhs) {
			return lhs.amount / rhs.amount;
		} else {
			return lhs.amount * screen_unit_to_pixels_factor<screen_unit_type_lhs> / rhs.amount *
				   screen_unit_to_pixels_factor<screen_unit_type_rhs>;
		}
	}

#define PROP_OP(OP)                                                                                                    \
	template <Screen_unit_type screen_unit_type_lhs, Screen_unit_type screen_unit_type_rhs>                            \
	[[nodiscard]] auto operator OP(prop::Screen_unit<screen_unit_type_lhs> lhs,                                        \
								   prop::Screen_unit<screen_unit_type_rhs> rhs) noexcept {                             \
		if constexpr (screen_unit_type_lhs == screen_unit_type_rhs) {                                                  \
			return lhs.amount OP rhs.amount;                                                                           \
		} else {                                                                                                       \
			return lhs.amount * screen_unit_to_pixels_factor<screen_unit_type_lhs> OP rhs.amount *                     \
				   screen_unit_to_pixels_factor<screen_unit_type_rhs>;                                                 \
		}                                                                                                              \
	}

	PROP_OP(<=>)
	PROP_OP(<)
	PROP_OP(<=)
	PROP_OP(>)
	PROP_OP(>=)
	PROP_OP(==)
	PROP_OP(!=)
#undef PROP_OP

	using Pixels = Screen_unit<Screen_unit_type::pixels>;
	using X_millimeters = Screen_unit<Screen_unit_type::x_millimeters>;
	using Y_millimeters = Screen_unit<Screen_unit_type::y_millimeters>;
	using Points = Screen_unit<Screen_unit_type::points>;
	using Text_sizes = Screen_unit<Screen_unit_type::text_sizes>;
	using Screen_width_percents = Screen_unit<Screen_unit_type::screen_width_percents>;
	using Screen_height_percents = Screen_unit<Screen_unit_type::screen_height_percents>;
	using Screen_size_percents = Screen_unit<Screen_unit_type::screen_size_percents>;

} // namespace prop

#define PROP_UDL(PROP_TYPE, PROP_UNIT)                                                                                 \
	namespace prop::literals {                                                                                         \
		inline namespace Screen_units_literals {                                                                       \
			[[nodiscard]] constexpr inline auto operator""##_##PROP_UNIT(long double value) {                          \
				return prop::PROP_TYPE{static_cast<Screen_unit_precision>(value)};                                     \
			}                                                                                                          \
			[[nodiscard]] constexpr inline auto operator""##_##PROP_UNIT(unsigned long long int value) {               \
				return prop::PROP_TYPE{static_cast<Screen_unit_precision>(value)};                                     \
			}                                                                                                          \
		}                                                                                                              \
	}                                                                                                                  \
	[[nodiscard]] constexpr inline auto operator""##_prop_##PROP_UNIT(long double value) {                             \
		return prop::PROP_TYPE{static_cast<prop::Screen_unit_precision>(value)};                                       \
	}                                                                                                                  \
	[[nodiscard]] constexpr inline auto operator""##_prop_##PROP_UNIT(unsigned long long int value) {                  \
		return prop::PROP_TYPE{static_cast<prop::Screen_unit_precision>(value)};                                       \
	}                                                                                                                  \
	inline std::ostream &operator<<(std::ostream &os, prop::PROP_TYPE value) {                                         \
		return os << value.amount << #PROP_UNIT;                                                                       \
	}                                                                                                                  \
	template <>                                                                                                        \
	struct std::formatter<prop::PROP_TYPE> : std::formatter<prop::Screen_unit_precision> {                             \
		template <class FmtContext>                                                                                    \
		auto format(prop::PROP_TYPE var, FmtContext &ctx) const {                                                      \
			auto it = std::formatter<prop::Screen_unit_precision>::format(var.amount, ctx);                            \
			for (char c : std::string_view{#PROP_UNIT}) {                                                              \
				*it = c;                                                                                               \
				++it;                                                                                                  \
			}                                                                                                          \
			return it;                                                                                                 \
		}                                                                                                              \
	};
PROP_UDL(Pixels, px)
PROP_UDL(X_millimeters, xmm)
PROP_UDL(Y_millimeters, ymm)
PROP_UDL(Points, pt)
PROP_UDL(Text_sizes, ts)
PROP_UDL(Screen_width_percents, swp)
PROP_UDL(Screen_height_percents, shp)
PROP_UDL(Screen_size_percents, ssp)
#undef PROP_UDL

namespace prop {
	enum class Screen_dimension_type { scalar, xpos, ypos, width, height };

	namespace detail {
		struct Operator_set {
			Screen_dimension_type left;
			Screen_dimension_type right;
			Screen_dimension_type result = left;
		};
		constexpr auto addable = std::to_array<Operator_set>({
			{Screen_dimension_type::xpos, Screen_dimension_type::width},
			{Screen_dimension_type::ypos, Screen_dimension_type::height},
			{Screen_dimension_type::width, Screen_dimension_type::xpos, Screen_dimension_type::xpos},
			{Screen_dimension_type::height, Screen_dimension_type::ypos, Screen_dimension_type::ypos},
			{Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::height, Screen_dimension_type::height},
		});
		constexpr auto subtractable = std::to_array<Operator_set>({
			{Screen_dimension_type::xpos, Screen_dimension_type::width},
			{Screen_dimension_type::ypos, Screen_dimension_type::height},
			{Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::height, Screen_dimension_type::height},
			{Screen_dimension_type::xpos, Screen_dimension_type::xpos, Screen_dimension_type::width},
			{Screen_dimension_type::ypos, Screen_dimension_type::ypos, Screen_dimension_type::height},
		});
		constexpr auto multiplicable = std::to_array<Operator_set>({
			{Screen_dimension_type::width, Screen_dimension_type::scalar},
			{Screen_dimension_type::height, Screen_dimension_type::scalar},
			{Screen_dimension_type::scalar, Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::scalar, Screen_dimension_type::height, Screen_dimension_type::height},
		});
		constexpr auto dividable = std::to_array<Operator_set>({
			{Screen_dimension_type::width, Screen_dimension_type::scalar},
			{Screen_dimension_type::height, Screen_dimension_type::scalar},
			{Screen_dimension_type::scalar, Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::scalar, Screen_dimension_type::height, Screen_dimension_type::height},
			{Screen_dimension_type::width, Screen_dimension_type::width, Screen_dimension_type::scalar},
			{Screen_dimension_type::height, Screen_dimension_type::height, Screen_dimension_type::scalar},
		});

		template <Screen_dimension_type left, Screen_dimension_type right, Screen_dimension_type result,
				  auto &operator_set>
		consteval bool get_has_operator() {
			for (auto &op_set : operator_set) {
				if (op_set.left == left and op_set.right == right and op_set.result == result) {
					return true;
				}
			}
			return false;
		}
		template <Screen_dimension_type left, Screen_dimension_type right, Screen_dimension_type result,
				  auto &operator_set>
		constexpr static bool has_operator = get_has_operator<left, right, result, operator_set>();

		template <Screen_dimension_type screen_dimension_type>
			requires(screen_dimension_type != Screen_dimension_type::scalar)
		struct Screen_dimension;

		template <Screen_dimension_type left, Screen_dimension_type right, auto &operator_set, std::size_t i>
		consteval auto get_result_of_i() {
			if constexpr (i == std::size(operator_set)) {
				return;
			}
			if constexpr (operator_set[i].left == left and operator_set[i].right == right) {
				return std::conditional_t<operator_set[i].result == Screen_dimension_type::scalar,
										  Screen_unit_precision, Screen_dimension<operator_set[i].result>>{};
			} else {
				return get_result_of_i<left, right, operator_set, i + 1>();
			}
		}

		template <Screen_dimension_type left, Screen_dimension_type right, auto &operator_set>
		consteval auto get_result_of() {
			return get_result_of_i<left, right, operator_set, 0>();
		}

		template <Screen_dimension_type left, Screen_dimension_type right, auto &operator_set>
		using Result_of = decltype(get_result_of<left, right, operator_set>());

		template <Screen_dimension_type screen_dimension_type>
			requires(screen_dimension_type != Screen_dimension_type::scalar)
		struct Screen_dimension {
			Pixels amount{};
#define PROP_OP(OP, LIST)                                                                                              \
	template <Screen_dimension_type other_screen_dimension_type>                                                       \
		requires detail::has_operator<screen_dimension_type, other_screen_dimension_type, screen_dimension_type,       \
									  detail::LIST>                                                                    \
	constexpr Screen_dimension &operator OP(Screen_dimension<screen_dimension_type> other) noexcept {                  \
		amount OP other.amount;                                                                                        \
		return *this;                                                                                                  \
	}
			PROP_OP(+=, addable)
			PROP_OP(-=, subtractable)
#undef PROP_OP
			template <Screen_unit_type screen_unit_type>
			constexpr Screen_dimension &operator=(Screen_unit<screen_unit_type> screen_unit) noexcept {
				amount = screen_unit;
				return *this;
			}
		};
#define PROP_X(PROP_OP, PROP_LIST)                                                                                     \
	template <Screen_dimension_type left, Screen_dimension_type right>                                                 \
	[[nodiscard]] constexpr Result_of<left, right, PROP_LIST> operator PROP_OP(Screen_dimension<left> lhs,             \
																			   Screen_dimension<right> rhs) noexcept { \
		return {lhs.amount PROP_OP rhs.amount};                                                                        \
	}
		PROP_X(+, addable)
		PROP_X(-, subtractable)
		PROP_X(*, multiplicable)
		PROP_X(/, dividable)
#undef PROP_X

#define PROP_X(PROP_OP)                                                                                                \
	template <Screen_dimension_type left, Screen_unit_type right>                                                      \
	[[nodiscard]] constexpr Screen_dimension<left> operator PROP_OP(Screen_dimension<left> lhs,                        \
																	Screen_unit<right> rhs) noexcept {                 \
		return {lhs.amount PROP_OP rhs};                                                                               \
	}
		PROP_X(+)
		PROP_X(-)
#undef PROP_X

#define PROP_OP(OP)                                                                                                    \
	template <Screen_dimension_type screen_dimension_type, Screen_unit_type screen_unit_type>                          \
	[[nodiscard]] auto operator OP(Screen_dimension<screen_dimension_type> lhs,                                        \
								   Screen_unit<screen_unit_type> rhs) noexcept {                                       \
		return lhs.amount OP rhs;                                                                                      \
	}                                                                                                                  \
	template <Screen_dimension_type screen_dimension_type, Screen_unit_type screen_unit_type>                          \
	[[nodiscard]] auto operator OP(Screen_unit<screen_unit_type> lhs,                                                  \
								   Screen_dimension<screen_dimension_type> rhs) noexcept {                             \
		return lhs OP rhs.amount;                                                                                      \
	}

		PROP_OP(<=>)
		PROP_OP(<)
		PROP_OP(<=)
		PROP_OP(>)
		PROP_OP(>=)
		PROP_OP(==)
		PROP_OP(!=)
#undef PROP_OP

		template <Screen_dimension_type screen_dimension_type, Screen_unit_type screen_unit_type>
		constexpr Screen_dimension<screen_dimension_type> &
		operator+=(Screen_dimension<screen_dimension_type> &screen_dimension,
				   Screen_unit<screen_unit_type> screen_unit) noexcept {
			screen_dimension.amount += screen_unit.amount * screen_unit_to_pixels_factor<screen_unit_type>;
			return screen_dimension;
		}
		template <Screen_dimension_type screen_dimension_type, Screen_unit_type screen_unit_type>
		constexpr Screen_dimension<screen_dimension_type> &
		operator-=(Screen_dimension<screen_dimension_type> &screen_dimension,
				   Screen_unit<screen_unit_type> screen_unit) noexcept {
			screen_dimension.amount -= screen_unit.amount * screen_unit_to_pixels_factor<screen_unit_type>;
			return screen_dimension;
		}
	} // namespace detail

	template <Screen_dimension_type screen_dimension_type>
	using Screen_dimension = std::conditional_t<screen_dimension_type == Screen_dimension_type::scalar,
												Screen_unit_precision, detail::Screen_dimension<screen_dimension_type>>;

	using Width = prop::Screen_dimension<prop::Screen_dimension_type::width>;
	using Height = prop::Screen_dimension<prop::Screen_dimension_type::height>;
	using Xpos = prop::Screen_dimension<prop::Screen_dimension_type::xpos>;
	using Ypos = prop::Screen_dimension<prop::Screen_dimension_type::ypos>;
} // namespace prop

#if __GNUG__
#pragma GCC diagnostic pop
#endif
