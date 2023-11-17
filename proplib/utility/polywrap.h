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

#include <cassert>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "type_name.h"
#include "type_traits.h"

namespace prop {
	template <class T>
	class Polywrap;

	namespace detail {
		template <class T>
		constexpr bool is_polywrap_v = is_type_specialization_v<std::remove_cvref_t<T>, prop::Polywrap>;

		template <class U, class T>
		concept Compatible_polywrap_value = std::convertible_to<U, T> && !
		is_polywrap_v<U>;

		template <class U, class T>
		concept Compatible_polywrap_pointer = requires(U &&u) {
												  { *u } -> Compatible_polywrap_value<T>;
											  };

		template <class U, class T>
		concept Compatible_polywrap = is_polywrap_v<U> && requires(U &&u) {
															  { *u.get() } -> Compatible_polywrap_value<T>;
														  };
		template <class Settee_type, class Setter_type>
		concept Settable = requires(Settee_type &&s) { std::declval<Setter_type>().set(std::forward<Settee_type>(s)); };

		template <class Derived, class Base>
		concept Has_slicing_problem =
			std::is_convertible_v<Derived *, Base *> && std::is_copy_constructible_v<Base> && !
		std::is_copy_constructible_v<Derived>;
	} // namespace detail

	template <class T>
	class Polywrap {
		public:
		Polywrap() = default;
		Polywrap(prop::detail::Settable<Polywrap<T>> auto &&u);

		T *get();
		const T *get() const;
		operator T *();
		operator const T *() const;
		explicit operator bool() const;
		T *operator->();
		const T *operator->() const;

		void set(detail::Compatible_polywrap_value<T> auto &&v);
		void set(detail::Compatible_polywrap_pointer<T> auto &&p);
		void set(detail::Compatible_polywrap<T> auto &&v);
		void set(std::nullptr_t);

		Polywrap &operator=(prop::detail::Settable<Polywrap<T>> auto &&u);

		private:
		std::shared_ptr<T> value_ptr;
	};

	struct Copy_error : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	namespace detail {
		template <class T>
		struct T_holder_base {
			T_holder_base(T *ptr)
				: ptr{ptr} {}
			virtual T_holder_base *clone();
			virtual ~T_holder_base() = default;
			T *ptr = nullptr;
		};

		template <class T, class U>
		struct T_holder : T_holder_base<T> {
			T_holder(U &&u)
				: T_holder_base<T>{&value}
				, value{std::move(u)} {}
			T_holder(const U &u)
				requires std::is_copy_constructible_v<U>
				: T_holder_base<T>{&value}
				, value{u} {}
			T_holder<T, U> *clone() override {
				if constexpr (std::is_copy_constructible_v<U>) {
					return new T_holder(value);
				} else {
					throw prop::Copy_error{"Attempt to copy a Polywrap<" + std::string{prop::type_name<T>()} +
										   "> holding a " + std::string{prop::type_name<U>()} +
										   " which is not copy-constructible"};
				}
			}
			U value;
		};

		template <class T, class Ptr>
		struct T_adopter : T_holder_base<T> {
			void operator()(T *ptr) {
				assert(ptr == &*this->ptr);
				if constexpr (std::is_assignable_v<Ptr, std::nullptr_t>) {
					ptr = nullptr;
				}
			}
			T_adopter(Ptr &&ptr)
				: T_holder_base<T>{&*ptr}
				, ptr{std::move(ptr)} {}
			T_adopter(const Ptr &ptr)
				: T_holder_base<T>{&*ptr}
				, ptr{ptr} {}
			Ptr ptr;
		};

		template <class T>
		struct T_deleter {
			void operator()(T *ptr) {
				assert(ptr == holder->ptr);
				holder = nullptr;
			}
			std::unique_ptr<T_holder_base<T>> holder;
		};

		template <class T>
		T_holder_base<T> *T_holder_base<T>::clone() {
			return new T_holder<T, T>(*ptr);
		}

		template <class T>
		auto polywrap_type_for(T &&t) {
			if constexpr (prop::is_dereferenceable_v<T>) {
				return *t;
			} else if constexpr (is_polywrap_v<T>) {
				return *t.get();
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
	Polywrap<T>::Polywrap(prop::detail::Settable<Polywrap<T>> auto &&u) {
		set(std::forward<decltype(u)>(u));
	}

	template <class T>
	T *Polywrap<T>::get() {
		static_assert(!std::is_reference_v<T>);
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
	void Polywrap<T>::set(detail::Compatible_polywrap_value<T> auto &&v) {
		using U = std::remove_cvref_t<decltype(v)>;
		if (value_ptr.use_count() == 1) { //attempt to avoid reallocation
			if constexpr (std::is_polymorphic_v<T>) {
				if (const auto deleter_ptr = std::get_deleter<prop::detail::T_deleter<T>>(value_ptr)) {
					if (dynamic_cast<prop::detail::T_holder<T, U> *>(deleter_ptr->holder.get())) {
						*value_ptr = std::forward<decltype(v)>(v);
						return;
					}
				}
			} else {
				*value_ptr = std::forward<decltype(v)>(v);
				return;
			}
		}
		//reallocate
		prop::detail::T_deleter<T> deleter{
			std::make_unique<prop::detail::T_holder<T, U>>(std::forward<decltype(v)>(v))};
		T *tp = deleter.holder->ptr;
		value_ptr = std::shared_ptr<T>(tp, std::move(deleter));
	}

	template <class T>
	void Polywrap<T>::set(detail::Compatible_polywrap_pointer<T> auto &&p) {
		using U = std::remove_cvref_t<decltype(p)>;
		if constexpr (prop::is_template_specialization_v<U, std::shared_ptr>) { //adopt shared_ptr
			value_ptr = std::forward<decltype(p)>(p);
		} else { //let original pointer handle ownership
			value_ptr = std::shared_ptr<T>(&*p, prop::detail::T_adopter<T, U>{std::forward<decltype(p)>(p)});
		}
	}

	template <class T>
	void Polywrap<T>::set(std::nullptr_t) {
		value_ptr = nullptr;
	}

	template <class T>
	Polywrap<T> &Polywrap<T>::operator=(prop::detail::Settable<Polywrap<T>> auto &&u) {
		set(std::forward<decltype(u)>(u));
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
