#include "property_details.h"
#include "property.h"

#ifdef PROPERTY_NAMES
#include <sstream>

std::string prop::detail::to_string(const void *p) {
	std::stringstream stream;
	stream << p;
	return stream.str();
}
#endif

#ifdef PROPERTY_DEBUG
#include <algorithm>
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
	~Tracer() {
		os << prop::Color::static_text << " in " << prop::Color::function_name
		   << function_part(source_location.function_name(), Function_part::name) << prop::Color::reset << '\n';
	}
	std::ostream &os;
	std::source_location source_location;
};

#define TRACE(...) Tracer{} << __VA_ARGS__

static std::string propnames(const prop::detail::Binding_set &set) {
	std::stringstream ss;
	ss << prop::Color::static_text << '[';
	const char *separator = "";
	for (const auto &binding : set.set) {
		ss << prop::Color::static_text << separator;
		separator = ", ";
		ss << binding->get_name();
	}
	ss << prop::Color::static_text << ']';
	return ss.str();
}
#else
#define TRACE(...)
#endif

void prop::detail::Property_base::update() {
	if (need_update) {
		need_update = false;
		if (this != current_binding) {
			update();
		}
	}
}

void prop::detail::Property_base::unbind() {
	clear_implicit_dependencies();
}

void prop::detail::Property_base::unbind_depends() {
	for (auto dependent = dependents.stable_iterator(); dependent; ++dependent) {
		if (dependent->implicit_dependencies.has(this)) {
			dependent->unbind();
		} else {
			for (auto &explicit_dependency : dependent->explicit_dependencies) {
				if (explicit_dependency == this) {
					explicit_dependency = nullptr;
				}
			}
		}
	}
}

void prop::detail::Property_base::read_notify() {
	if (current_binding && dependents.add(current_binding)) {
		TRACE("Added      " << get_name() << " as an implicit dependency of " << current_binding->get_name());
		current_binding->implicit_dependencies.add(this);
	}
}

void prop::detail::Property_base::read_notify() const {
	const_cast<prop::detail::Property_base *>(this)->read_notify();
}

void prop::detail::Property_base::write_notify() {
	TRACE("Notifying  " << get_name() << "->" << propnames(dependents));
	for (const auto &dependent : dependents.set) {
		dependent->need_update = true;
	}
	if (not dependents.set.empty()) {
		for (auto dependent = dependents.stable_iterator(); dependent; ++dependent) {
			dependent->Property_base::update();
		}
	}
}

void prop::detail::Property_base::update_start() {
	TRACE("Updating   " << get_name());
	previous_binding = current_binding;
	clear_implicit_dependencies();
	current_binding = this;
}

void prop::detail::Property_base::update_complete() {
	TRACE("Completed  " << get_name());
	current_binding = previous_binding;
}

void prop::detail::Property_base::set_explicit_dependencies(Binding_list list) {
	for (auto dependency : explicit_dependencies) {
		if (dependency && !implicit_dependencies.has(dependency)) {
			dependency->dependents.remove(this);
		}
	}
	explicit_dependencies = std::move(list);
	for (auto dependency : explicit_dependencies) {
		if (dependency) {
			dependency->dependents.add(this);
		}
	}
}

bool prop::detail::Property_base::is_implicit_dependency_of(const Property_base &other) const {
	return other.implicit_dependencies.has(this) and not other.explicit_dependencies.has(this);
}

bool prop::detail::Property_base::is_explicit_dependency_of(const Property_base &other) const {
	return other.explicit_dependencies.has(this);
}

bool prop::detail::Property_base::is_dependency_of(const Property_base &other) const {
	return dependents.has(&other);
}

bool prop::detail::Property_base::is_implicit_dependent_of(const Property_base &other) const {
	return implicit_dependencies.has(&other) and not explicit_dependencies.has(&other);
}

bool prop::detail::Property_base::is_explicit_dependent_of(const Property_base &other) const {
	return explicit_dependencies.has(&other);
}

bool prop::detail::Property_base::is_dependent_on(const Property_base &other) const {
	return other.dependents.has(this);
}

prop::detail::Property_base::Property_base
#ifdef PROPERTY_NAMES
	(std::string_view type_name)
	: type{type_name}

{
	TRACE("Created    " << get_name() << prop::Color::static_text
						<< (creation_binding ? " with creation binding " + creation_binding->get_name() : ""));
#else
	() {
	TRACE("Created    " << get_name() << prop::Color::static_text
						<< (creation_binding ? " with creation binding " + prop::to_string(creation_binding) : ""));
#endif
}

prop::detail::Property_base::Property_base(const prop::detail::Binding_set &bs) {
	set_explicit_dependencies({.list = {std::begin(bs.set), std::end(bs.set)}});
}

prop::detail::Property_base::Property_base(Property_base &&other) {
	TRACE(other.get_name() << " split into " << "<" << other.get_name() << " (moved from) and " << ">"
						   << other.get_name() << " (moved to)");
#ifdef PROPERTY_DEBUG
	custom_name = ">" + other.custom_name;
	other.custom_name = "<" + other.custom_name;
#endif
	using std::swap;
	swap(explicit_dependencies, other.explicit_dependencies);
	swap(implicit_dependencies, other.implicit_dependencies);
	swap(dependents, other.dependents);

	for (auto dependent : dependents.set) {
		dependent->explicit_dependencies.replace(&other, this);
		dependent->implicit_dependencies.replace(&other, this);
	}
}

void prop::detail::Property_base::operator=(Property_base &&other) {
	TRACE("Moving     " << other.get_name() << " to " << get_name());
	sever_implicit_dependents();
	other.sever_implicit_dependents();
	take_explicit_dependents(std::move(other));
}

prop::detail::Property_base::~Property_base() {
	TRACE("Destroying " << get_name());
	//clear implicit dependencies
	clear_implicit_dependencies();
	//clear explicit dependencies
	for (const auto &dependency : explicit_dependencies) {
		if (dependency) {
			TRACE("Removed    " << get_name() << " from dependents of " << dependency->get_name());
			dependency->dependents.remove(this);
		}
	}
	//clear creation dependency
	if (dependents.has(creation_binding)) {
		TRACE("Removed    " << get_name() << " as dependency from " << creation_binding->get_name() << " because "
							<< get_name() << " is a temporary getting destroyed");
		dependents.remove(creation_binding);
		creation_binding->implicit_dependencies.remove(this);
	}
	//remove dependents
	sever_implicit_dependents();
	if (not dependents.set.empty()) {
		for (auto dependent = dependents.stable_iterator(); dependent; ++dependent) {
			for (auto &explicit_dependency : dependent->explicit_dependencies) {
				if (explicit_dependency == this) {
#ifdef PROPERTY_DEBUG
					TRACE("Explicit dependency " << get_name() << " of " << dependent->get_name()
												 << " is no longer available");
#endif
					explicit_dependency = nullptr;
				}
			}
			dependent->update();
		}
	}
	TRACE("Destroyed  " << get_name());
#ifdef PROPERTY_DEBUG
	custom_name = "~" + custom_name;
#endif
}

void prop::detail::Property_base::clear_implicit_dependencies() {
	if (implicit_dependencies.is_empty()) {
		return;
	}
	TRACE("Clearing   " << get_name());
	for (const auto &dependency : implicit_dependencies.set) {
		TRACE("Removed    " << get_name() << " from dependents of " << dependency->get_name());
		assert(dependency->dependents.has(this));
		dependency->dependents.remove(this);
	}
	implicit_dependencies.clear();
}

void prop::detail::Property_base::sever_implicit_dependents() {
	if (dependents.is_empty()) {
		return;
	}
	TRACE("Severing   " << get_name());
	if (not dependents.set.empty()) {
		for (auto dependent = dependents.stable_iterator(); dependent; ++dependent) { //unbind dependents
			if (dependent->implicit_dependencies.has(this)) {
				if (dependent->explicit_dependencies.has(this)) {
					dependent->implicit_dependencies.remove(this);
				} else {
					TRACE("Unbound    " << dependent->get_name() << " because it implicitly depends on " << get_name()
										<< " which is getting severed");
					on_property_severed(&*dependent, this);
					dependent->unbind();
				}
			}
		}
	}
}

void prop::detail::Property_base::take_explicit_dependents(Property_base &&source) {
	if (source.dependents.is_empty()) {
		return;
	}
	TRACE(get_name());
	if (not source.dependents.set.empty()) {
		for (auto source_dependent = source.dependents.stable_iterator(); source_dependent; ++source_dependent) {
			if (source_dependent->explicit_dependencies.has(&source)) {
				TRACE(get_name() << " adopting explicit dependent " << source_dependent->get_name() << " from "
								 << source.get_name());
				dependents.add(&*source_dependent);
				for (auto &explicit_dependent : source_dependent->explicit_dependencies) {
					if (explicit_dependent == &source) {
						explicit_dependent = this;
					}
				}
				if (!source_dependent->implicit_dependencies.has(&source)) {
					source.dependents.remove(&*source_dependent);
				}
			}
		}
	}
}

const prop::detail::Binding_set &prop::detail::Property_base::get_implicit_dependencies() const {
	return implicit_dependencies;
}

const prop::detail::Binding_set &prop::detail::Property_base::get_dependents() const {
	return dependents;
}

void prop::detail::swap(Property_base &lhs, Property_base &rhs) {
	TRACE("Swapping   " << lhs.get_name() << " and " << rhs.get_name());
	using std::swap;
	swap(lhs.explicit_dependencies, rhs.explicit_dependencies);
	swap(lhs.implicit_dependencies, rhs.implicit_dependencies);
	swap(lhs.dependents, rhs.dependents);

	for (auto dependent : lhs.dependents.set) {
		dependent->explicit_dependencies.replace(&rhs, &lhs);
		dependent->implicit_dependencies.replace(&rhs, &lhs);
	}
	for (auto dependent : rhs.dependents.set) {
		dependent->explicit_dependencies.replace(&lhs, &rhs);
		dependent->implicit_dependencies.replace(&lhs, &rhs);
	}
}

bool prop::detail::Binding_set::has(const Property_base *pb) const {
	return set.contains(const_cast<Property_base *>(pb));
}

bool prop::detail::Binding_set::is_empty() const {
	return set.empty();
}

bool prop::detail::Binding_set::add(Property_base *pb) {
	return set.insert(pb).second;
}

void prop::detail::Binding_set::remove(Property_base *pb) {
	set.erase(pb);
}

void prop::detail::Binding_set::clear() {
	set.clear();
}

void prop::detail::Binding_set::replace(Property_base *old_value, Property_base *new_value) {
	if (has(old_value)) {
		remove(old_value);
		add(new_value);
	}
}

bool prop::detail::Binding_list::has(const Property_base *property) const {
	return std::find(std::begin(list), std::end(list), property) != std::end(list);
}

std::size_t prop::detail::Binding_list::size() const {
	return std::size(list);
}

void prop::detail::Binding_list::replace(Property_base *old_value, Property_base *new_value) {
	for (auto &value : list) {
		if (value == old_value) {
			value = new_value;
		}
	}
}

prop::detail::Property_base *prop::detail::Binding_list::operator[](std::size_t index) const {
	return list[index];
}

std::vector<prop::detail::Property_base *>::iterator prop::detail::Binding_list::begin() {
	return std::begin(list);
}

std::vector<prop::detail::Property_base *>::iterator prop::detail::Binding_list::end() {
	return std::end(list);
}

std::vector<prop::detail::Property_base *>::const_iterator prop::detail::Binding_list::begin() const {
	return std::begin(list);
}

std::vector<prop::detail::Property_base *>::const_iterator prop::detail::Binding_list::end() const {
	return std::end(list);
}

std::ostream &prop::detail::operator<<(std::ostream &os, const prop::detail::Binding_set &set) {
	const auto &static_text_color = prop::Color::static_text;
	const char *separator = "";
	for (const auto &element : set.set) {
		os << static_text_color << separator << prop::Color::reset;
#ifdef PROPERTY_NAMES
		os << element->get_name();
#else
		os << &element;
#endif
		separator = ", ";
	}
	return os;
}

std::ostream &prop::detail::operator<<(std::ostream &os, const prop::detail::Binding_list &list) {
	const auto &static_text_color = prop::Color::static_text;
	const char *separator = "";
	for (const auto &element : list) {
		os << static_text_color << separator << prop::Color::reset;
#ifdef PROPERTY_NAMES
		os << element->get_name();
#else
		os << &element;
#endif
		separator = ", ";
	}
	return os;
}

prop::detail::Binding_set::Stable_iterator::Stable_iterator(const prop::detail::Binding_set &binding_set)
	: base{binding_set}
	, cit{std::begin(base.explicit_dependencies)} {}

prop::detail::Binding_set::Stable_iterator::operator bool() const {
	return cit != std::end(base.explicit_dependencies);
}

prop::detail::Binding_set::Stable_iterator &prop::detail::Binding_set::Stable_iterator::operator++() {
	while (++cit != std::end(base.explicit_dependencies)) {
		if (*cit) {
			break;
		}
	}
	return *this;
}

prop::detail::Binding_set::Stable_iterator prop::detail::Binding_set::stable_iterator() const {
	return {*this};
}

prop::detail::Property_base &prop::detail::Binding_set::Stable_iterator::operator*() const {
	return **cit;
}

prop::detail::Property_base *prop::detail::Binding_set::Stable_iterator::operator->() const {
	return *cit;
}
