#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace prop {
	template <class T>
	class Polywrap {
		public:
		Polywrap() = default;
		template <class U>
		Polywrap(U &&u);
		Polywrap(Polywrap &&other);
		Polywrap(const Polywrap &&other);
		Polywrap(Polywrap &other);
		Polywrap(const Polywrap &other);
		T *get();
		const T *get() const;
		operator T *();
		operator const T *() const;
		explicit operator bool() const;

		void set(T &&t);
		void set(const T &t);
		void set(T *t);
		void set(std::shared_ptr<T> p);
		void set(std::nullptr_t);
		template <class U>
		void set(U &&u);
		template <class U>
		Polywrap &operator=(U &&u);

		private:
		std::shared_ptr<T> value_ptr;
	};
#define PROP_BINOPS PROP_X(<=>) PROP_X(==) PROP_X(!=) PROP_X(<) PROP_X(<=) PROP_X(>) PROP_X(>=)
#define PROP_X(OP)                                                                                                     \
	template <class T, class U>                                                                                        \
	auto operator OP(const Polywrap<T> &lhs, const U &rhs) {                                                           \
		return lhs.get() OP rhs;                                                                                       \
	}                                                                                                                  \
	template <class T, class U>                                                                                        \
	auto operator OP(const U &lhs, const Polywrap<T> &rhs) {                                                           \
		return lhs OP rhs.get();                                                                                       \
	}
	PROP_BINOPS
#undef PROP_X
#undef PROP_BINOPS
} // namespace prop

//detail
namespace prop {
	namespace detail {
		template <class T, class U>
		auto is_compatible_value_type(T t, U &&u) -> decltype(t = u, std::true_type{});
		std::false_type is_compatible_value_type(...);
		template <class T, class U>
		constexpr bool is_compatible_value_type_v =
			decltype(is_compatible_value_type(std::declval<T>(), std::declval<U>()))::value;
		template <class T, class U>
		auto is_compatible_pointer_type(T t, U &&u) -> decltype(t = *u, std::true_type{});
		std::false_type is_compatible_pointer_type(...);
		template <class T, class U>
		constexpr bool is_compatible_pointer_type_v =
			decltype(is_compatible_pointer_type(std::declval<T>(), std::declval<U>()))::value;
		template <class T>
		auto is_dereferenceable(T &&t) -> decltype(*t, std::true_type{});
		std::false_type is_dereferenceable(...);
		template <class T>
		constexpr bool is_dereferenceable_v = decltype(is_dereferenceable(std::declval<T>()))::value;
		template <class T>
		auto polywrap_type_for(T &&t) {
			if constexpr (is_dereferenceable_v<T>) {
				return *t;
			} else {
				return t;
			}
		}
		template <class T>
		using polywrap_type_for_t = decltype(polywrap_type_for(std::declval<T>()));
	} // namespace detail
	template <class T>
	Polywrap(T &&t) -> Polywrap<detail::polywrap_type_for_t<T>>;
} // namespace prop

//implementation
namespace prop {
	template <class T>
	template <class U>
	Polywrap<T>::Polywrap(U &&u) {
		set(std::forward<U>(u));
	}

	template <class T>
	Polywrap<T>::Polywrap(Polywrap &&other)
		: value_ptr(std::move(other.value_ptr)) {}

	template <class T>
	Polywrap<T>::Polywrap(const Polywrap &&other)
		: Polywrap{other} {}

	template <class T>
	Polywrap<T>::Polywrap(Polywrap &other)
		: Polywrap{std::as_const(other)} {}

	template <class T>
	Polywrap<T>::Polywrap(const Polywrap &other)
		: value_ptr{other.value_ptr ? std::make_shared<T>(*other.get()) : nullptr} {}

	template <class T>
	T *Polywrap<T>::get() {
		return value_ptr.get();
	}

	template <class T>
	const T *Polywrap<T>::get() const {
		return value_ptr.get();
	}

	template <class T>
	Polywrap<T>::operator bool() const {
		return value_ptr != nullptr;
	}

	template <class T>
	void Polywrap<T>::set(T &&t) {
		if (!value_ptr) {
			value_ptr = std::make_shared<T>(std::move(t));
		} else {
			*value_ptr = std::move(t);
		}
	}

	template <class T>
	void Polywrap<T>::set(const T &t) {
		if (!value_ptr) {
			value_ptr = std::make_shared<T>(t);
		} else {
			*value_ptr = std::move(t);
		}
	}

	template <class T>
	void Polywrap<T>::set(T *t) {
		value_ptr = std::shared_ptr<T>(
			t, +[](T *t) {});
	}

	template <class T>
	void Polywrap<T>::set(std::shared_ptr<T> p) {
		value_ptr = std::move(p);
	}

	template <class T>
	void Polywrap<T>::set(std::nullptr_t) {
		value_ptr = nullptr;
	}

	template <class T>
	template <class U>
	void Polywrap<T>::set(U &&u) {
		//value type
		if constexpr (detail::is_compatible_value_type_v<T, U>) {
			if (!value_ptr) {
				value_ptr = std::make_shared<T>(std::forward<U>(u));
			} else {
				*value_ptr = std::forward<U>(u);
			}
		}
		//pointer type
		else if constexpr (detail::is_compatible_pointer_type_v<T, U>) {
			T *t = &*u;
			value_ptr = std::shared_ptr<T>(t, [other_pointer = std::forward<U>(u)](T *) {});
		}
		//incompatible type
		else {
			static_assert(!sizeof(T), "Invalid type for Polywrap::set");
		}
	}

	template <class T>
	template <class U>
	Polywrap<T> &Polywrap<T>::operator=(U &&u) {
		this->set(std::forward<U>(u));
		return *this;
	}

	template <class T>
	Polywrap<T>::operator T *() {
		return get();
	}

	template <class T>
	Polywrap<T>::operator const T *() const {
		return get();
	}
} // namespace prop
