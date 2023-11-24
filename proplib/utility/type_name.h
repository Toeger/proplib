#pragma once

#include <string_view>

//printing type names, from https://stackoverflow.com/a/56766138

namespace prop {
	template <typename T>
	constexpr auto type_name() {
		std::string_view name, prefix, suffix;
#ifdef __clang__
		name = __PRETTY_FUNCTION__;
		prefix = "auto prop::type_name() [T = ";
		suffix = "]";
#elif defined(__GNUC__)
		name = __PRETTY_FUNCTION__;
		prefix = "constexpr auto prop::type_name() [with T = ";
		suffix = "]";
#elif defined(_MSC_VER)
		name = __FUNCSIG__;
		prefix = "auto __cdecl prop::type_name<";
		suffix = ">(void)";
#endif
		name.remove_prefix(prefix.size());
		name.remove_suffix(suffix.size());
		return name;
	}
} // namespace prop
