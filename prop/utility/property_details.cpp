#include "property_details.h"
#include "color.h"

#ifdef PROPERTY_DEBUG
#include <algorithm>
#include <cassert>
#include <concepts>
#include <iostream>
#include <source_location>

constexpr std::string_view path(std::string_view filepath) {
#ifdef WIN32
	const auto separator = '\\';
#else
	const auto separator = '/';
#endif
	auto pos = filepath.rfind(separator);
	if (pos != filepath.npos) {
		filepath.remove_suffix(filepath.length() - pos - 1);
	}
	return filepath;
}

constexpr std::string_view file(std::string_view filepath) {
#ifdef WIN32
	const auto separator = '\\';
#else
	const auto separator = '/';
#endif
	auto pos = filepath.rfind(separator);
	if (pos != filepath.npos) {
		filepath.remove_prefix(pos + 1);
	}
	return filepath;
}

enum class Function_part { before_name, name, after_name };

constexpr std::string_view function_part(std::string_view function, Function_part fp) {
	auto name_end_pos = function.find('(');
	if (name_end_pos == function.npos) {
		name_end_pos = function.size() - 1;
	}
	if (fp == Function_part::after_name) {
		function.remove_prefix(name_end_pos);
		return function;
	}

	auto name_start_pos = std::find_if(std::rbegin(function) + static_cast<long int>(function.size()) -
										   static_cast<long int>(name_end_pos),
									   std::rend(function), [](char c) { return c == ':' or c == ' '; }) -
						  std::rbegin(function);
	name_start_pos = static_cast<long int>(function.size()) - name_start_pos;
	if (fp == Function_part::before_name) {
		function.remove_suffix(function.size() - static_cast<std::size_t>(name_start_pos));
		return function;
	}
	function.remove_suffix(function.size() - name_end_pos);
	function.remove_prefix(static_cast<std::size_t>(name_start_pos));
	return function;
}

struct Tracer {
	Tracer(std::ostream &output_stream = std::clog, std::source_location sl = std::source_location::current())
		: os{output_stream}
		, source_location{sl} {
		//std::move(*this) << prop::Color::path << path(sl.file_name());
		//std::move(*this) << prop::Color::file << file(sl.file_name()) << ':' << prop::Color::file << sl.line();
		//std::move(*this) << ' ';
		//std::move(*this) << prop::Color::function_type << function_part(sl.function_name(), Function_part::before_name);
		//std::move(*this) << prop::Color::function_name << function_part(sl.function_name(), Function_part::name);
		//std::move(*this) << prop::Color::function_type << function_part(sl.function_name(), Function_part::after_name);
		//std::move(*this) << ": ";
	}
	Tracer &&operator<<(prop::Color color) && {
		os << color;
		return std::move(*this);
	}
	Tracer &&operator<<(prop::Color::Reset reset) && {
		os << reset;
		return std::move(*this);
	}
	Tracer &&operator<<(const char *s) && {
		os << prop::Color::static_text << s;
		return std::move(*this);
	}
	Tracer &&operator<<(char c) && {
		os << prop::Color::static_text << c;
		return std::move(*this);
	}
	Tracer &&operator<<(std::string_view s) && {
		os << s;
		return std::move(*this);
	}
	Tracer &&operator<<(std::convertible_to<int> auto i) && {
		os << i;
		return std::move(*this);
	}
	Tracer &&operator<<(const void *p) && {
		os << prop::Color::address << p;
		return std::move(*this);
	}
	Tracer &&operator<<(std::span<const prop::Property_link::Property_pointer> span) && {
		os << '[';
		const char *sep = "";
		for (auto &l : span) {
			os << prop::Color::type << l->type() << prop::Color::static_text << '@' << prop::Color::address
			   << l.get_pointer() << prop::Color::static_text << sep;
			sep = ",";
		}
		os << ']';
		return std::move(*this);
	}
	~Tracer() {
		os << prop::Color::static_text << " in " << prop::Color::function_name
		   << function_part(source_location.function_name(), Function_part::name) << prop::Color::reset << '\n';
	}
	std::ostream &os;
	std::source_location source_location;
};
#endif
