#pragma once

#include <type_traits>
#include <utility>

#include "type_list.h"

namespace prop {
	//helpers to extract callable information
	namespace detail {
		//is callable
		template <class Return_type, class... Params>
		auto get_is_callable(Return_type (*)(Params...)) -> std::true_type;
		template <class Return_type, class Class_type, class... Params>
		auto get_is_callable(Return_type (Class_type::*)(Params...)) -> std::true_type;
		template <class Return_type, class Class_type, class... Params>
		auto get_is_callable(Return_type (Class_type::*)(Params...) const) -> std::true_type;
		template <class T>
		auto get_is_callable(const T &) -> decltype(get_is_callable(&std::remove_reference_t<T>::operator()));
		template <class T>
		auto get_is_callable(const T &&) -> decltype(get_is_callable(&std::remove_reference_t<T>::operator()));
		std::false_type get_is_callable(...);

		//return type
		template <class Return_type, class... Params>
		auto get_return_type(Return_type (*)(Params...)) -> Return_type;
		template <class Return_type, class Class_type, class... Params>
		auto get_return_type(Return_type (Class_type::*)(Params...)) -> Return_type;
		template <class Return_type, class Class_type, class... Params>
		auto get_return_type(Return_type (Class_type::*)(Params...) const) -> Return_type;
		template <class T>
		auto get_return_type(T &&) -> decltype(get_return_type(&std::remove_reference_t<T>::operator()));
		template <class T>
		using Return_type = decltype(get_return_type(std::declval<T>()));

		//class type
		template <class Return_type, class Class_type, class... Params>
		auto get_class_type(Return_type (Class_type::*)(Params...)) -> Class_type;
		template <class Return_type, class Class_type, class... Params>
		auto get_class_type(Return_type (Class_type::*)(Params...) const) -> Class_type;
		template <class T>
		auto get_class_type(T &&) -> decltype(get_class_type(&std::remove_reference_t<T>::operator()));
		template <class T>
		using Class_type = decltype(get_class_type(std::declval<T>()));
		template <class T>
		auto get_has_class_type(T &&t) -> decltype(get_class_type(std::forward<T>(t)), std::true_type{});
		std::false_type get_has_class_type(...);
		template <class T>
		constexpr bool has_class_type = decltype(get_has_class_type(std::declval<T>()))::value;

		//arguments
		template <class Return_type, class... Params>
		auto get_Params(Return_type (*)(Params...)) -> Type_list<Params...>;
		template <class Return_type, class Class_type, class... Params>
		auto get_Params(Return_type (Class_type::*)(Params...)) -> Type_list<Params...>;
		template <class Return_type, class Class_type, class... Params>
		auto get_Params(Return_type (Class_type::*)(Params...) const) -> Type_list<Params...>;
		template <class T>
		auto get_Params(T &&) -> decltype(get_Params(&std::remove_reference_t<T>::operator()));
		template <class T>
		using Params = decltype(get_Params(std::declval<T>()));

		//get function pointer
		template <class Return_type, class... Params>
		auto get_function_pointer(Type_list<Params...>) -> Return_type (*)(Params...);
		template <class Return_type, class Class, class... Params>
		auto get_member_function_pointer(Type_list<Params...>) -> Return_type (Class::*)(Params...);
		template <class Return_type, class Class, class... Params>
		auto get_const_member_function_pointer(Type_list<Params...>) -> Return_type (Class::*)(Params...) const;

		enum class Callable_type { none, class_, function };
		template <class T>
		consteval Callable_type get_callable_type() {
			if constexpr (not decltype(get_is_callable(std::declval<T>()))::value) {
				return Callable_type::none;
			}
			if constexpr (prop::detail::has_class_type<T>) {
				return Callable_type::class_;
			} else {
				return Callable_type::function;
			}
		}
	} // namespace detail

	template <class T>
	static constexpr bool is_callable_v = decltype(prop::detail::get_is_callable(std::declval<T>()))::value;

	//struct to hold callable information
	template <class T, detail::Callable_type type>
	struct Callable_info;

	template <class T>
	struct Callable_info<T, detail::Callable_type::class_> {
		Callable_info(T &&) {}
		Callable_info(const T &) {}
		Callable_info() = default;
		using Return_type = prop::detail::Return_type<T>;
		using Class_type = prop::detail::Class_type<T>;
		constexpr static bool has_class_type = true;
		using Params = prop::detail::Params<T>;
		using as_function_pointer =
			decltype(prop::detail::get_member_function_pointer<Return_type, Class_type>(Params{}));
		using as_non_member_function_pointer = decltype(prop::detail::get_function_pointer<Return_type>(Params{}));
	};

	template <class T>
	struct Callable_info<T, detail::Callable_type::function> {
		Callable_info(T &&) {}
		Callable_info(const T &) {}
		Callable_info() = default;
		using Return_type = prop::detail::Return_type<T>;
		constexpr static bool has_class_type = false;
		using Params = prop::detail::Params<T>;
		using as_function_pointer = decltype(prop::detail::get_function_pointer<Return_type>(Params{}));
		using as_non_member_function_pointer = decltype(prop::detail::get_function_pointer<Return_type>(Params{}));
	};

	template <class T>
	struct Callable_info<T, detail::Callable_type::none> {
		Callable_info(T &&) {}
		Callable_info(const T &) {}
		Callable_info() = default;
	};

	template <class T>
	Callable_info(T &&) -> Callable_info<std::remove_reference_t<T>, detail::get_callable_type<T>()>;

	template <class T>
	using Callable_info_for = decltype(Callable_info(std::declval<T>()));

	//helper to make a function pointer
	template <class Return_type, class Typelist>
	using function_pointer = decltype(prop::detail::get_function_pointer<Return_type>(Typelist{}));
	template <class Return_type, class Class, class Typelist>
	using member_function_pointer = decltype(prop::detail::get_member_function_pointer<Return_type, Class>(Typelist{}));
	template <class Return_type, class Class, class Typelist>
	using const_member_function_pointer =
		decltype(prop::detail::get_const_member_function_pointer<Return_type, Class>(Typelist{}));

	//make overloaded functions
	template <class... Callables>
	struct Overload : Callables... {
		Overload(Callables &&...callables)
			: Callables(std::forward<Callables>(callables))... {}
		using Callables::operator()...;
	};
} // namespace prop
