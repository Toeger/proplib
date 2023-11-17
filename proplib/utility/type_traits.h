#pragma once

#include <type_traits>

namespace prop {
	template <class T, template <class...> class Template>
	struct is_template_specialization : std::false_type {};
	template <template <class...> class Template, class... Args>
	struct is_template_specialization<Template<Args...>, Template> : std::true_type {};
	template <class T, template <class...> class Template>
	constexpr bool is_template_specialization_v = is_template_specialization<T, Template>::value;

	template <class T>
	auto is_dereferenceable(T &&t) -> decltype(*std::declval<T>(), std::true_type{});
	std::false_type is_dereferenceable(...);
	template <class T>
	constexpr bool is_dereferenceable_v = decltype(is_dereferenceable(std::declval<T>()))::value;

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

	template <class T, template <class...> class Template>
	struct is_type_specialization : std::false_type {};
	template <template <class...> class Template, class... Args>
	struct is_type_specialization<Template<Args...>, Template> : std::true_type {};
	template <class T, template <class...> class Template>
	constexpr bool is_type_specialization_v = is_type_specialization<T, Template>::value;
} // namespace prop
