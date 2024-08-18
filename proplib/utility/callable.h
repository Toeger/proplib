#include <type_traits>
#include <utility>

#include "type_list.h"

namespace prop {
	//helpers to extract callable information
	namespace detail {
		//is callable
		template <class Return_type, class... Args>
		auto get_is_callable(Return_type (*)(Args...)) -> std::true_type;
		template <class Return_type, class Class_type, class... Args>
		auto get_is_callable(Return_type (Class_type::*)(Args...)) -> std::true_type;
		template <class Return_type, class Class_type, class... Args>
		auto get_is_callable(Return_type (Class_type::*)(Args...) const) -> std::true_type;
		template <class T>
		auto get_is_callable(const T &) -> decltype(get_is_callable(&std::remove_reference_t<T>::operator()));
		template <class T>
		auto get_is_callable(const T &&) -> decltype(get_is_callable(&std::remove_reference_t<T>::operator()));
		std::false_type get_is_callable(...);

		//return type
		template <class Return_type, class... Args>
		auto get_return_type(Return_type (*)(Args...)) -> Return_type;
		template <class Return_type, class Class_type, class... Args>
		auto get_return_type(Return_type (Class_type::*)(Args...)) -> Return_type;
		template <class Return_type, class Class_type, class... Args>
		auto get_return_type(Return_type (Class_type::*)(Args...) const) -> Return_type;
		template <class T>
		auto get_return_type(T &&) -> decltype(get_return_type(&std::remove_reference_t<T>::operator()));
		template <class T>
		using Return_type = decltype(get_return_type(std::declval<T>()));

		//class type
		template <class Return_type, class Class_type, class... Args>
		auto get_class_type(Return_type (Class_type::*)(Args...)) -> Class_type;
		template <class Return_type, class Class_type, class... Args>
		auto get_class_type(Return_type (Class_type::*)(Args...) const) -> Class_type;
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
		template <class Return_type, class... Args>
		auto get_args(Return_type (*)(Args...)) -> Type_list<Args...>;
		template <class Return_type, class Class_type, class... Args>
		auto get_args(Return_type (Class_type::*)(Args...)) -> Type_list<Args...>;
		template <class Return_type, class Class_type, class... Args>
		auto get_args(Return_type (Class_type::*)(Args...) const) -> Type_list<Args...>;
		template <class T>
		auto get_args(T &&) -> decltype(get_args(&std::remove_reference_t<T>::operator()));
		template <class T>
		using Args = decltype(get_args(std::declval<T>()));

		//get function pointer
		template <class Return_type, class... Args>
		auto get_function_pointer(Type_list<Args...>) -> Return_type (*)(Args...);
		template <class Return_type, class Class, class... Args>
		auto get_member_function_pointer(Type_list<Args...>) -> Return_type (Class::*)(Args...);
		template <class Return_type, class Class, class... Args>
		auto get_const_member_function_pointer(Type_list<Args...>) -> Return_type (Class::*)(Args...) const;
	} // namespace detail

	template <class T>
	static constexpr bool is_callable_v = decltype(detail::get_is_callable(std::declval<T>()))::value;

	//struct to hold callable information
	template <class T, bool has_class>
	struct Callable_info;

	template <class T>
	struct Callable_info<T, true> {
		Callable_info(T &&) {}
		Callable_info(const T &) {}
		Callable_info() = default;
		using Return_type = detail::Return_type<T>;
		using Class_type = detail::Class_type<T>;
		constexpr static bool has_class_type = true;
		using Args = detail::Args<T>;
		using as_function_pointer = decltype(detail::get_member_function_pointer<Return_type, Class_type>(Args{}));
		using as_non_member_function_pointer = decltype(detail::get_function_pointer<Return_type>(Args{}));
	};

	template <class T>
	struct Callable_info<T, false> {
		Callable_info(T &&) {}
		Callable_info(const T &) {}
		Callable_info() = default;
		using Return_type = detail::Return_type<T>;
		constexpr static bool has_class_type = false;
		using Args = detail::Args<T>;
		using as_function_pointer = decltype(detail::get_function_pointer<Return_type>(Args{}));
		using as_non_member_function_pointer = decltype(detail::get_function_pointer<Return_type>(Args{}));
	};

	template <class T>
	Callable_info(T &&) -> Callable_info<std::remove_reference_t<T>, detail::has_class_type<T>>;

	template <class T>
	using Callable_info_for = decltype(Callable_info(std::declval<T>()));

	//helper to make a function pointer
	template <class Return_type, class Typelist>
	using function_pointer = decltype(detail::get_function_pointer<Return_type>(Typelist{}));
	template <class Return_type, class Class, class Typelist>
	using member_function_pointer = decltype(detail::get_member_function_pointer<Return_type, Class>(Typelist{}));
	template <class Return_type, class Class, class Typelist>
	using const_member_function_pointer =
		decltype(detail::get_const_member_function_pointer<Return_type, Class>(Typelist{}));

	//make overloaded functions
	template <class... Callables>
	struct Overload : Callables... {
		Overload(Callables &&...callables)
			: Callables(std::forward<Callables>(callables))... {}
		using Callables::operator()...;
	};
} // namespace prop
