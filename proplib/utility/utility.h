#pragma once

#include <algorithm>
#include <concepts>
#include <utility>

namespace prop {
	namespace utility {
		struct {
			template <class T>
			static void operator()(T &lhs, T &rhs) {
				using std::swap;
				swap(lhs, rhs);
			}
		} static swap;
	} // namespace utility

	class Widget;

	struct Self {
		template <class T>
			requires(std::derived_from<T, prop::Widget>)
		operator T &() {
			return dynamic_cast<T &>(*self);
		}
		template <class T>
			requires(std::derived_from<T, prop::Widget>)
		operator const T &() const {
			return dynamic_cast<T &>(*self);
		}
		Self(Widget *w)
			: self{w} {}
		void operator=(Widget *w) {
			self = w;
		}
		bool operator==(const prop::Widget &w) const {
			return self == &w;
		}

		prop::Widget *self;
	};

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

} // namespace prop
