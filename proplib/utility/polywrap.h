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
#include <type_traits>
#include <utility>

#include "exceptions.h"
#include "type_name.h"
#include "type_traits.h"

namespace prop {
	template <class T>
	class Polywrap;

	namespace detail {
		template <class Polywrap>
		constexpr bool is_polywrap_v =
			prop::is_template_specialization_v<std::remove_cvref_t<Polywrap>, prop::Polywrap>;

		template <class Compatible_type, class Polywrap_inner_type>
		concept Compatible_polywrap_value =
			std::convertible_to<Compatible_type, Polywrap_inner_type> && !is_polywrap_v<Compatible_type>;

		template <class Compatible_pointer, class Polywrap_inner_type>
		concept Compatible_polywrap_pointer = !is_polywrap_v<Compatible_pointer> && requires(Compatible_pointer &&ptr) {
			{ *ptr } -> std::convertible_to<Polywrap_inner_type &>;
		};

		template <class Polywrap, class Polywrap_inner_type>
		concept Compatible_polywrap =
			is_polywrap_v<Polywrap> && (!std::is_lvalue_reference_v<Polywrap> || requires(Polywrap &&pw) {
				{ *pw.get() } -> Compatible_polywrap_value<Polywrap_inner_type>;
			});
		template <class Settee_type, class Setter_type>
		concept Settable = requires(Settee_type &&s) { std::declval<Setter_type>().set(std::forward<Settee_type>(s)); };
	} // namespace detail

	template <class T>
	class Polywrap {
		public:
		Polywrap() = default;
		Polywrap(const Polywrap &other)
			requires(std::is_copy_constructible_v<T>);
		Polywrap(Polywrap &&other);
		Polywrap(prop::detail::Settable<Polywrap<T>> auto &&u);

		Polywrap &operator=(const Polywrap &other)
			requires(std::is_copy_constructible_v<T>);
		Polywrap &operator=(Polywrap &&other);
		Polywrap &operator=(prop::detail::Settable<Polywrap<T>> auto &&u);

		T *get();
		const T *get() const;
		operator T *();
		operator const T *() const;
		explicit operator bool() const;
		T *operator->();
		const T *operator->() const;

		void set(prop::detail::Compatible_polywrap_value<T> auto &&v);
		void set(prop::detail::Compatible_polywrap_pointer<T> auto &&p);
		void set(prop::detail::Compatible_polywrap<T> auto &&v);
		void set(std::nullptr_t);

		private:
		std::shared_ptr<T> value_ptr;
	};

	namespace detail {
		template <class T>
		struct T_holder_base {
			T_holder_base(T *ptr_)
				: ptr{ptr_} {}
			virtual T_holder_base<T> *clone();
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
					throw prop::Copy_error{"Attempted to copy a prop::Polywrap<" + std::string{prop::type_name<T>()} +
										   "> holding a " + std::string{prop::type_name<U>()} +
										   " which is not copy-constructible"};
				}
			}
			U value;
		};

		template <class T, class Ptr>
		struct T_adopter : T_holder_base<T> {
			void operator()(T *ptr_) {
				assert(ptr_ == &*this->ptr);
				if constexpr (std::is_assignable_v<Ptr, std::nullptr_t>) {
					ptr = nullptr;
				}
			}
			T_adopter(Ptr &&ptr_)
				: T_holder_base<T>{&*ptr_}
				, ptr{std::move(ptr_)} {}
			T_adopter(const Ptr &ptr_)
				: T_holder_base<T>{&*ptr_}
				, ptr{ptr_} {}
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
			if constexpr (std::is_copy_constructible_v<T>) {
				return new T_holder<T, T>(*ptr);
			} else {
				throw prop::Copy_error{"Attempted to copy a prop::Polywrap<" + std::string{prop::type_name<T>()} +
									   "> holding a " + std::string{prop::type_name<T>()} +
									   " which is not copy-constructible"};
			}
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
		requires(!std::is_same_v<Polywrap<T>, U>)                                                                      \
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
	Polywrap<T>::Polywrap(const Polywrap &other)
		requires(std::is_copy_constructible_v<T>)
	{
		set(other);
	}

	template <class T>
	Polywrap<T>::Polywrap(Polywrap &&other) {
		set(std::move(other));
	}

	template <class T>
	Polywrap<T>::Polywrap(prop::detail::Settable<Polywrap<T>> auto &&u) {
		set(std::forward<decltype(u)>(u));
	}

	template <class T>
	Polywrap<T> &Polywrap<T>::operator=(const Polywrap<T> &other)
		requires(std::is_copy_constructible_v<T>)
	{
		set(other);
		return *this;
	}

	template <class T>
	Polywrap<T> &Polywrap<T>::operator=(Polywrap<T> &&other) {
		set(std::forward<decltype(other)>(other));
		return *this;
	}

	template <class T>
	Polywrap<T> &Polywrap<T>::operator=(prop::detail::Settable<Polywrap<T>> auto &&u) {
		set(std::forward<decltype(u)>(u));
		return *this;
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
	void Polywrap<T>::set(prop::detail::Compatible_polywrap_value<T> auto &&v) {
		using U = std::remove_cvref_t<decltype(v)>;
		if (value_ptr.use_count() == 1) { //attempt to avoid reallocation
			if constexpr (std::is_polymorphic_v<T>) {
				if constexpr (std::is_move_assignable_v<U>) {
					if (const auto deleter_ptr = std::get_deleter<prop::detail::T_deleter<T>>(value_ptr)) {
						if (dynamic_cast<prop::detail::T_holder<T, U> *>(deleter_ptr->holder.get())) {
							*static_cast<U *>(value_ptr.get()) = std::forward<decltype(v)>(v);
							return;
						}
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
	void Polywrap<T>::set(prop::detail::Compatible_polywrap_pointer<T> auto &&p) {
		using U = std::remove_cvref_t<decltype(p)>;
		if constexpr (prop::is_template_specialization_v<U, std::shared_ptr>) { //adopt shared_ptr
			value_ptr = std::forward<decltype(p)>(p);
		} else { //let original pointer handle ownership
			T *ptr = &*p;
			value_ptr = std::shared_ptr<T>(ptr, prop::detail::T_adopter<T, U>{std::forward<decltype(p)>(p)});
		}
	}

	template <class T>
	void Polywrap<T>::set(prop::detail::Compatible_polywrap<T> auto &&v) {
		if constexpr (std::is_rvalue_reference_v<decltype(v)>) {
			std::swap(value_ptr, v.value_ptr);
			return;
		} else if constexpr (!std::is_polymorphic_v<T>) {
			set(*v.value_ptr);
			return;
		} else {
			using U = decltype(v.value_ptr)::element_type;
			if (const auto deleter_ptr = std::get_deleter<prop::detail::T_deleter<T>>(v.value_ptr)) {
				prop::detail::T_holder_base<T> *holder = deleter_ptr->holder->clone();
				prop::detail::T_deleter<T> deleter{std::unique_ptr<prop::detail::T_holder_base<T>>(holder)};
				T *ptr = deleter.holder->ptr;
				value_ptr = std::shared_ptr<T>(ptr, std::move(deleter));
				if (dynamic_cast<prop::detail::T_holder<T, U> *>(deleter_ptr->holder.get())) {
					*value_ptr = *v.value_ptr;
					return;
				}
			} else {
				set(*v.value_ptr);
			}
		}
	}

	template <class T>
	void Polywrap<T>::set(std::nullptr_t) {
		value_ptr = nullptr;
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
