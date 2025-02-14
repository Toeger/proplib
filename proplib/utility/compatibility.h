#pragma once

#include <utility>

namespace prop {
	namespace std {
		using namespace ::std;
#ifndef __cpp_lib_forward_like
		//std::forward_like for clang-18
		template <class T, class U>
			requires true
		constexpr auto &&forward_like(U &&x) noexcept {
			constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
			if constexpr (std::is_lvalue_reference_v<T &&>) {
				if constexpr (is_adding_const)
					return std::as_const(x);
				else
					return static_cast<U &>(x);
			} else {
				if constexpr (is_adding_const)
					return std::move(std::as_const(x));
				else
					return std::move(x);
			}
		}
#endif
	} // namespace std
} // namespace prop
