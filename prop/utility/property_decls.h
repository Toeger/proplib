#pragma once

#include "prop/utility/required_pointer.h"

#include <functional>
#include <span>
#include <type_traits>

namespace prop {
	template <class T>
	class Property;

	enum struct Updater_result : char { unchanged, changed, unbind };

	class Property_link;
	namespace detail {
		template <class T>
		std::move_only_function<prop::Updater_result(prop::Property<T> &,
													 std::span<const prop::Required_pointer<Property_link>>)>
		get_binding_function(T *);
		std::move_only_function<prop::Updater_result(std::span<const prop::Required_pointer<Property_link>>)>
		get_binding_function(void *);
		template <class T>
		using binding_function_t = decltype(get_binding_function(std::declval<T *>()));

		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T, void>)
		detail::binding_function_t<T> create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(std::is_same_v<T, void>)
		detail::binding_function_t<void> create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
	} // namespace detail
} // namespace prop
