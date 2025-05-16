#pragma once

#include <algorithm>
#include <concepts>
#include <sstream>
#include <type_traits>
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

	template <class T>
	std::string to_string(const T &t)
		requires(requires(std::stringstream &ss) { ss << t; })
	{
		std::stringstream ss;
		ss << t;
		return std::move(ss).str();
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
} // namespace prop
