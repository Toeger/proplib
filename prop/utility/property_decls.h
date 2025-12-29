#pragma once

#include "prop/utility/required_pointer.h"

#include <functional>
#include <span>
#include <type_traits>

namespace prop {
	template <class T>
	class Property;

	enum struct Updater_result : char { unchanged, changed, sever };

	class Property_link;
	namespace detail {
		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T, void>)
		std::move_only_function<prop::Updater_result(
			prop::Property<T> &, std::span<const prop::Required_pointer<Property_link>> explicit_dependencies)>
		create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
	}
} // namespace prop
