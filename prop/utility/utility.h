#pragma once

#include "prop/platform/external/magic_enum.hpp"
#include <algorithm>
#include <ranges>
#include <sstream>
#include <type_traits>
#include <utility>

namespace prop {
	namespace utility {
		struct {
			template <class T>
			[[maybe_unused]] static void operator()(T &lhs, T &rhs) {
				using std::swap;
				swap(lhs, rhs);
			}
		} static swap;
	} // namespace utility

	template <class T>
	std::make_signed_t<T> signed_cast(T t) {
		return static_cast<std::make_signed_t<T>>(t);
	}
	template <class T>
	std::make_unsigned_t<T> unsigned_cast(T t) {
		return static_cast<std::make_unsigned_t<T>>(t);
	}

	template <class Container>
	bool contains(const Container &c, const typename Container::value_type &v) {
		return std::find(std::begin(c), std::end(c), v) != std::end(c);
	}

	template <class T>
	std::string to_string(const T &t)
		requires(requires(std::stringstream &ss) { ss << t; })
	{
		if constexpr (requires { t == nullptr; }) {
			if (t == nullptr) {
				return "nullptr";
			}
		}
		std::stringstream ss;
		ss << t;
		return std::move(ss).str();
	}

	template <class T>
	std::string to_display_string(const T &t, const std::size_t max_length = static_cast<std::size_t>(-1)) {
		if (max_length == 0) {
			return "";
		}
		std::stringstream ss;
		if constexpr (std::is_same_v<T, unsigned char> or std::is_same_v<T, signed char>) {
			ss << static_cast<int>(t);
		} else if constexpr (std::is_same_v<T, char>) {
			if (t >= ' ' && t <= '~') {
				ss << '\'' << t << '\'';
			} else {
				ss << static_cast<int>(t);
			}
		} else if constexpr (std::is_same_v<T, bool>) {
			ss << std::boolalpha << t;
		} else if constexpr (std::is_enum_v<T>) {
			ss << magic_enum::enum_name(t);
		} else if constexpr (std::is_same_v<T, std::string> or std::is_same_v<T, std::string_view> or
							 std::is_same_v<T, char *> or std::is_same_v<T, const char *>) {
			ss << '"' << t << '"';
		} else if constexpr (requires { ss << t; }) {
			ss << t;
		} else if constexpr (std::is_same_v<T, void>) {
			ss << std::boolalpha << t;
		} else if constexpr (requires { requires std::ranges::input_range<T>; }) {
			ss << '[';
			bool first = true;
			for (const auto &e : t) {
				if (static_cast<std::size_t>(ss.tellp()) < max_length) {
					break;
				}
				if (!first) {
					ss << ", ";
				} else {
					first = false;
				}
				ss << to_display_string(e, max_length - static_cast<std::size_t>(ss.tellp()));
			}
			ss << ']';
		} else {
			ss << "<void>";
		}
		auto result = std::move(ss).str();
		if (result.size() > max_length) {
			result.resize(max_length - 1);
			result.push_back('~');
		}
		return result;
	}

	template <class From, class To>
	using copy_cv_t =
		std::conditional_t<std::is_volatile_v<From>, volatile std::conditional_t<std::is_const_v<From>, const To, To>,
						   std::conditional_t<std::is_const_v<From>, const To, To>>;
	template <class From, class To>
	using copy_ref_t = std::conditional_t<std::is_lvalue_reference_v<From>, To &,
										  std::conditional_t<std::is_rvalue_reference_v<From>, To &&, To>>;

	template <class From, class To>
	using copy_cvref_t = copy_ref_t<From, copy_cv_t<std::remove_reference_t<From>, std::remove_cvref_t<To>>>;

	template <class T, class U>
	concept Match_cvr = std::is_same_v<U, copy_cv_t<T, U>>;

	constexpr std::string to_string(std::size_t size) {
		if (size == 0) {
			return "0";
		}
		std::string retval;
		while (size) {
			retval += (size % 10) + '0';
			size /= 10;
		}
		std::reverse(std::begin(retval), std::end(retval));
		return retval;
	}
} // namespace prop
