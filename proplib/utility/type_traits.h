#pragma once

#include <type_traits>

namespace prop {
	template <class T, template <class...> class Template>
	struct is_template_specialization : std::false_type {};
	template <template <class...> class Template, class... Args>
	struct is_template_specialization<Template<Args...>, Template> : std::true_type {};
	template <class T, template <class...> class Template>
	constexpr bool is_template_specialization_v = is_template_specialization<T, Template>::value;
} // namespace prop
