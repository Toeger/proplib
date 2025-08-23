#pragma once

#include <iterator>
#include <type_traits>

#if __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

namespace prop {
#ifndef PROP_SCREEN_UNIT_PRECISION
#define PROP_SCREEN_UNIT_PRECISION float
#endif
	using Screen_unit_precision = PROP_SCREEN_UNIT_PRECISION;

	enum class Screen_unit_type {
		pixels,
		millimeters,
		points,
		screen_width_percents,
		screen_height_percents,
		screen_size_percents,
	};
	template <Screen_unit_type screen_unit_type>
	inline long double screen_unit_to_pixels_factor = 0;
	template <>
	constexpr inline long double screen_unit_to_pixels_factor<Screen_unit_type::pixels> = 1;

	void reload_screen_dimensions();

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
			amount = other_unit.amount * screen_unit_to_pixels_factor<other_screen_unit_type> /
					 screen_unit_to_pixels_factor<screen_unit_type>;
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
	using Millimeters = Screen_unit<Screen_unit_type::millimeters>;
	using Points = Screen_unit<Screen_unit_type::points>;
	using Screen_width_percents = Screen_unit<Screen_unit_type::screen_width_percents>;
	using Screen_height_percents = Screen_unit<Screen_unit_type::screen_height_percents>;
	using Screen_size_percents = Screen_unit<Screen_unit_type::screen_size_percents>;

	namespace literals {
		inline namespace Screen_units_literals {
#define PROP_UDL(NAME, UDL)                                                                                            \
	[[nodiscard]] constexpr inline auto operator""##UDL(long double value) {                                           \
		return NAME{static_cast<Screen_unit_precision>(value)};                                                        \
	}                                                                                                                  \
	[[nodiscard]] constexpr inline auto operator""##UDL(unsigned long long int value) {                                \
		return NAME{static_cast<Screen_unit_precision>(value)};                                                        \
	}

			PROP_UDL(Pixels, _px)
			PROP_UDL(Millimeters, _mm)
			PROP_UDL(Points, _pt)
			PROP_UDL(Screen_width_percents, _swp)
			PROP_UDL(Screen_height_percents, _shp)
			PROP_UDL(Screen_size_percents, _ssp)
#undef PROP_UDL
		} // namespace Screen_units_literals
	} // namespace literals

	enum class Screen_dimension_type { scalar, xpos, ypos, width, height };

	namespace detail {
		struct Operator_set {
			Screen_dimension_type left;
			Screen_dimension_type right;
			Screen_dimension_type result = left;
		};
		constexpr Operator_set addable[] = {
			{Screen_dimension_type::xpos, Screen_dimension_type::width},
			{Screen_dimension_type::ypos, Screen_dimension_type::height},
			{Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::height, Screen_dimension_type::height},
		};
		constexpr Operator_set subtractable[] = {
			{Screen_dimension_type::xpos, Screen_dimension_type::width},
			{Screen_dimension_type::ypos, Screen_dimension_type::height},
			{Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::height, Screen_dimension_type::height},
			{Screen_dimension_type::xpos, Screen_dimension_type::xpos, Screen_dimension_type::width},
			{Screen_dimension_type::ypos, Screen_dimension_type::ypos, Screen_dimension_type::height},
		};
		constexpr Operator_set multiplicable[] = {
			{Screen_dimension_type::width, Screen_dimension_type::scalar},
			{Screen_dimension_type::height, Screen_dimension_type::scalar},
			{Screen_dimension_type::scalar, Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::scalar, Screen_dimension_type::height, Screen_dimension_type::height},
		};
		constexpr Operator_set dividable[] = {
			{Screen_dimension_type::width, Screen_dimension_type::scalar},
			{Screen_dimension_type::height, Screen_dimension_type::scalar},
			{Screen_dimension_type::scalar, Screen_dimension_type::width, Screen_dimension_type::width},
			{Screen_dimension_type::scalar, Screen_dimension_type::height, Screen_dimension_type::height},
			{Screen_dimension_type::width, Screen_dimension_type::width, Screen_dimension_type::scalar},
			{Screen_dimension_type::height, Screen_dimension_type::height, Screen_dimension_type::scalar},
		};

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
