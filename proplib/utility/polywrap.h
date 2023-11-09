#pragma once

/*
Polywrap<T>
Purpose 1:
	Polymorphous wrapper around a type T. Like any other pointer, smart pointer and reference, the static type can be a base class while the dynamic type can
	be a derived type, allowing virtual function calls to the base be overridden by the derived class.
	Additionally, the derived class will be destroyed properly, even if the base class does not have a virtual destructor.
Purpose 2:
	Abstract away ownership and cleanup. Polywrap<T> is compatible with std::unique_ptr<T>, std::shared_ptr<T>, T* (non-owner) and other smart pointers that
	produce a T& or compatible type when dereferenced as well as T and things derived from T while preserving ownership and cleanup semantics.
	If you pass an std::unique_ptr with a custom deleter, that custom deleter will be called appropriately.
 */

#include <memory>
#include <type_traits>
#include <utility>

#include "type_traits.h"

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
		T *operator->();
		const T *operator->() const;

		void set(T &&t);
		void set(const T &t);
		void set(T *t);
		void set(std::shared_ptr<T> p);
		void set(std::nullptr_t);
		template <class U>
		auto set(U &&u)
			-> std::enable_if_t<prop::is_compatible_value_type_v<T, U> || prop::is_compatible_pointer_type_v<T, U>>;
		template <class U>
		Polywrap &operator=(U &&u);

		private:
		std::shared_ptr<T> value_ptr;
	};

	namespace detail {
		template <class T>
		auto polywrap_type_for(T &&t) {
			if constexpr (prop::is_dereferenceable_v<T>) {
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
	T *Polywrap<T>::operator->() {
		return value_ptr.get();
	}

	template <class T>
	const T *Polywrap<T>::operator->() const {
		return value_ptr.get();
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
	auto Polywrap<T>::set(U &&u)
		-> std::enable_if_t<prop::is_compatible_value_type_v<T, U> || prop::is_compatible_pointer_type_v<T, U>> {
		if constexpr (prop::is_compatible_value_type_v<T, U>) {
			if (!value_ptr) {
				value_ptr = std::make_shared<T>(std::forward<U>(u));
			} else {
				*value_ptr = std::forward<U>(u);
			}
		} else if constexpr (prop::is_compatible_pointer_type_v<T, U>) {
			//TODO: treat Polywraps special
			T *t = &*u;
			value_ptr = std::shared_ptr<T>(t, [other_pointer = std::forward<U>(u)](T *) {});
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
