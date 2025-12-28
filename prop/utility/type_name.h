#pragma once

#include <string_view>

//printing type names, from https://stackoverflow.com/a/56766138

namespace PROP_DETAIL { //separate namespace because all types in this namespace don't display the namespace.
	//For example PROP_DETAIL::foo would display as only foo. We don't want that for prop::.
	template <typename T>
	consteval auto type_name() {
		std::string_view name, prefix, suffix;
#ifdef __clang__
		name = __PRETTY_FUNCTION__;
		prefix = "auto PROP_DETAIL::type_name() [T = ";
		suffix = "]";
#elif defined(__GNUC__)
		name = __PRETTY_FUNCTION__;
		prefix = "consteval auto PROP_DETAIL::type_name() [with T = ";
		suffix = "]";
#elif defined(_MSC_VER)
		name = __FUNCSIG__;
		prefix = "auto __cdecl PROP_DETAIL::type_name<";
		suffix = ">(void)";
#endif
		name.remove_prefix(prefix.size());
		name.remove_suffix(suffix.size());
		if (name == "std::basic_string<char>") {
			return std::string_view{"std::string"};
		} else if (name == "std::__cxx11::basic_string<char>") {
			return std::string_view{"std::string"};
		}
		return name;
	}
} // namespace PROP_DETAIL

namespace prop {
	template <typename T>
	consteval auto type_name() {
		return PROP_DETAIL::type_name<T>();
	}
} // namespace prop
