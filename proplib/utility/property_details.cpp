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
	Tracer &&operator<<(std::span<const prop::detail::Property_link> span) && {
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

#define TRACE(...) Tracer{} << __VA_ARGS__
#else
#define TRACE(...)
#endif

void prop::detail::Property_base::read_notify() const {
	if (current_binding and current_binding != this and not current_binding->has_dependency(*this)) {
		TRACE("Added      " << get_name() << " as an implicit dependency of\n           "
							<< current_binding->get_name());
		current_binding->add_implicit_dependency({this, true});
	}
}

void prop::detail::Property_base::write_notify() {
	if (explicit_dependencies + implicit_dependencies == dependencies.size()) {
		return;
	}
	TRACE("Notifying  " << get_name() << "->" << get_dependents());
	for (auto &dependent : get_stable_dependents()) {
		dependent->Property_base::update();
	}
}

void prop::detail::Property_base::update_start(Property_base *&previous_binding) {
	TRACE("Updating   " << get_name());
	previous_binding = current_binding;
	for (std::size_t i = explicit_dependencies; i < explicit_dependencies + implicit_dependencies; i++) {
		dependencies[i]->remove_dependent(*this);
	}
	dependencies.erase(std::begin(dependencies) + explicit_dependencies,
					   std::begin(dependencies) + explicit_dependencies + implicit_dependencies);
	current_binding = this;
}

void prop::detail::Property_base::update_complete(Property_base *&previous_binding) {
	TRACE("Completed  " << get_name());
	current_binding = previous_binding;
}

prop::detail::Property_base::Property_base() {
	TRACE("Created    " << this);
}

prop::detail::Property_base::Property_base(std::string_view type) {
	TRACE("Created    " << prop::Color::type << type << prop::Color::static_text << '@' << prop::Color::address << this
						<< prop::Color::reset);
}

prop::detail::Property_base::Property_base(std::vector<prop::detail::Property_link> initial_explicit_dependencies)
	: dependencies{std::move(initial_explicit_dependencies)} {
	TRACE("Created    " << this);
	for (auto &explicit_dependency : dependencies) {
		if (auto ptr = explicit_dependency.get_pointer()) {
			ptr->add_dependent(*this);
		}
	}
}

prop::detail::Property_base::Property_base(Property_base &&other) {
	TRACE("Moved " << other.get_name() << " from  " << &other << " to " << this);
#ifdef PROPERTY_DEBUG
	custom_name = std::move(other.custom_name);
	other.custom_name = "<moved from>";
#endif
	using std::swap;
	swap(explicit_dependencies, other.explicit_dependencies);
	swap(implicit_dependencies, other.implicit_dependencies);
	swap(dependencies, other.dependencies);

	for (std::size_t i = 0; i < explicit_dependencies + implicit_dependencies; ++i) { //dependencies
		if (auto dependency_ptr = dependencies[i].get_pointer()) {
			dependency_ptr->replace_dependent(other, *this);
			break;
		}
	}
	for (std::size_t i = explicit_dependencies + implicit_dependencies; i < std::size(dependencies); ++i) {
		auto &dependent = *dependencies[i];
		for (std::size_t dependent_dependencies_index = 0;
			 dependent_dependencies_index < dependent.explicit_dependencies + dependent.implicit_dependencies;
			 dependent_dependencies_index++) {
			auto &dep = dependent.dependencies[dependent_dependencies_index];
			if (dep == &other) {
				dep = this;
				if (dependent_dependencies_index >= dependent.explicit_dependencies) {
					break;
				}
			}
		}
	}
}

void prop::detail::Property_base::operator=(Property_base &&other) {
	TRACE("Moving     " << other.get_name() << " to " << get_name());
	swap(*this, other);
}

prop::detail::Property_base::~Property_base() {
	TRACE("Destroying " << this);
	for (std::size_t i = 0; i < explicit_dependencies + implicit_dependencies; ++i) {
		dependencies[i]->remove_dependent(*this);
		TRACE("Removed    " << get_name() << " from dependents of " << dependencies[i]->get_name());
	}
	for (std::size_t dependent_index = explicit_dependencies + implicit_dependencies;
		 dependent_index < std::size(dependencies); ++dependent_index) {
		auto &dependent = *dependencies[dependent_index];
		for (std::uint16_t dependent_dependency_index = 0;
			 dependent_dependency_index < dependent.explicit_dependencies + dependent.implicit_dependencies;
			 ++dependent_dependency_index) {
			auto &dependent_dependency = dependent.dependencies[dependent_dependency_index];
			if (dependent_dependency == this) {
				if (dependent_dependency.is_required()) {
					dependent.unbind();
					TRACE("Removed    " << get_name() << " from required dependencies of " << dependent.get_name());
					break; //all dependents and dependencies are gone
				}
				if (dependent_dependency_index < dependent.explicit_dependencies) { //explicit dependency
					dependent_dependency = nullptr;
					TRACE("Removed    " << get_name() << " from optional explicit dependencies of "
										<< dependent.get_name());
					//duplicate explicit dependency possible
				} else { //implicit dependency
					dependent.dependencies.erase(std::begin(dependent.dependencies) + dependent_dependency_index);
					TRACE("Removed    " << get_name() << " from optional implicit dependencies of "
										<< dependent.get_name());
					break; //only 1 implicit dependency possible
				}
			}
		}
	}
	TRACE("Destroyed  " << this);
#ifdef PROPERTY_DEBUG
	custom_name = "~" + custom_name;
#endif
}

void prop::detail::swap(Property_base &lhs, Property_base &rhs) {
	//TODO: Surely there is a simpler way to do this
	TRACE("Swapping   " << lhs.get_name() << " and " << rhs.get_name());
	if (lhs.dependencies.empty() and rhs.dependencies.empty()) {
		return;
	}
	struct Tmp final : Property_base {
		Tmp()
			: Property_base(prop::type_name<Tmp>()) {}
		std::string_view type() const override {
			return prop::type_name<Tmp>();
		}
		std::string value_string() const override {
			return "Tmp";
		}
		bool has_source() const override {
			return false;
		}
	} static sentinel;

	auto replace = [](Property_base &pb_old, Property_base &pb_new) {
		assert(pb_new.dependencies.empty());
		assert(pb_new.explicit_dependencies == 0);
		assert(pb_new.implicit_dependencies == 0);
		assert(pb_old.explicit_dependencies + pb_old.implicit_dependencies <= pb_old.dependencies.size());
		if (pb_old.dependencies.empty()) {
			return;
		}
		while (pb_old.explicit_dependencies) {
			auto &dependency = pb_old.dependencies[pb_new.dependencies.size()];
			dependency->replace_dependent(pb_old, pb_new);
			pb_new.dependencies.push_back(dependency);
			pb_new.explicit_dependencies++;
			pb_old.explicit_dependencies--;
		}
		while (pb_old.implicit_dependencies) {
			auto &dependency = pb_old.dependencies[pb_new.dependencies.size()];
			dependency->replace_dependent(pb_old, pb_new);
			pb_new.dependencies.push_back(dependency);
			pb_new.implicit_dependencies++;
			pb_old.implicit_dependencies--;
		}
		while (pb_old.dependencies.size() > pb_new.dependencies.size()) {
			auto &dependent_link = pb_old.dependencies[pb_new.dependencies.size()];
			auto &dependent = *dependent_link;
			for (std::size_t i = 0; i < dependent.explicit_dependencies + dependent.implicit_dependencies; i++) {
				if (dependent.dependencies[i] == &pb_old) {
					dependent.dependencies[i] = &pb_new;
				}
			}
			pb_new.dependencies.push_back(dependent_link);
		}
		pb_old.dependencies.clear();
	};
	replace(lhs, sentinel);
	replace(rhs, lhs);
	replace(sentinel, rhs);
}

std::string_view prop::detail::Property_base::type() const {
	return prop::type_name<decltype(*this)>();
}

void prop::detail::Property_base::set_explicit_dependencies(std::vector<Property_link> deps) {
	assert(deps.size() < std::numeric_limits<decltype(explicit_dependencies)>::max());
	if (dependencies.empty()) {
		TRACE("Setting    " << prop::Color::type << type() << "@" << this << "'s explicit dependencies to\n           "
							<< deps);
		if (dependencies.capacity() > deps.capacity()) {
			std::copy(std::begin(deps), std::end(deps), std::back_inserter(dependencies));
		} else {
			dependencies = std::move(deps);
		}
		for (const auto &dependency : dependencies) {
			dependency->add_dependent(*this);
		}
		explicit_dependencies = static_cast<decltype(explicit_dependencies)>(dependencies.size());
		return;
	}
	if (deps.empty()) {
		TRACE("Removing   " << prop::Color::type << type() << "@" << this << "'s explicit dependencies "
							<< dependencies);
		for (std::size_t i = 0; i < explicit_dependencies; i++) {
			dependencies[i]->remove_dependent(*this);
		}
		dependencies.erase(std::begin(dependencies), std::begin(dependencies) + explicit_dependencies);
		explicit_dependencies = 0;
		return;
	}
	TRACE("Replacing  " << prop::Color::type << type() << "@" << this << "'s explicit dependencies:\n           old: "
						<< dependencies << "\n           new: " << deps);
	for (std::size_t i = 0, end = std::min<std::size_t>(explicit_dependencies, deps.size()); i < end; i++) {
		if (dependencies[i].get_pointer() == deps[i].get_pointer()) {
			dependencies[i] = deps[i];
			continue;
		}
		dependencies[i]->remove_dependent(*this);
		dependencies[i] = deps[i];
		dependencies[i]->add_dependent(*this);
	}
	while (explicit_dependencies < deps.size()) {
		dependencies.insert(std::begin(deps) + explicit_dependencies, deps[explicit_dependencies]);
		dependencies[explicit_dependencies++]->add_dependent(*this);
	}
	if (explicit_dependencies > deps.size()) {
		for (std::size_t i = deps.size(); i < explicit_dependencies; i++) {
			dependencies[i]->remove_dependent(*this);
		}
		dependencies.erase(std::begin(deps) + explicit_dependencies,
						   std::begin(deps) + static_cast<decltype(explicit_dependencies)>(deps.size()));
		explicit_dependencies = static_cast<decltype(explicit_dependencies)>(deps.size());
	}
}

void prop::detail::Property_base::print_extended_status(const prop::detail::Extended_status_data &esd,
														int current_depth) const {
	std::string indent;
	for (int i = 0; i < current_depth; i++) {
		indent += esd.indent_with;
	}
	auto print_dep = [&esd, &indent, current_depth](std::span<const prop::detail::Property_link> deps) {
		if (deps.empty()) {
			esd.output << "[]\n";
		} else {
			esd.output << prop::Color::reset << "[";
			const char *sep = "";
			if (esd.depth == current_depth) {
				for (auto &dep : deps) {
					if (dep) {
						esd.output << prop::Color::static_text << sep << prop::Color::type << dep->type()
								   << prop::Color::static_text << '(' << prop::Color::reset << dep->value_string()
								   << prop::Color::reset << ")@" << prop::Color::address << dep;
						sep = ", ";
					} else {
						esd.output << prop::Color::reset << "null";
					}
				}
				esd.output << prop::Color::reset << "]\n";
			} else {
				esd.output << '\n';
				for (auto &ed : deps) {
					esd.output << prop::Color::reset << sep;
					sep = ",\n";
					ed->print_extended_status(esd, current_depth + 1);
				}
				esd.output << indent << "           " << prop::Color::reset << "]\n";
			}
		}
	};
	esd.output << indent << prop::Color::static_text;
	if (current_depth) {
		esd.output << "           Property: ";
	} else {
		esd.output << "Property   ";
	}
#ifdef PROPERTY_NAMES
	esd.output << get_name() << '\n';
#else
	esd.output << prop::Color::address << this << '\n';
#endif
	esd.output << indent << prop::Color::static_text << "           value: " << prop::Color::reset << value_string()
			   << "\n";
	esd.output << indent << prop::Color::static_text << "           source: " << prop::Color::reset
			   << (has_source() ? "Yes" : "No") << "\n";
	esd.output << indent << prop::Color::static_text << "           Explicit dependencies: " << prop::Color::reset;
	print_dep(get_explicit_dependencies());
	esd.output << indent << prop::Color::static_text << "           Implicit dependencies: " << prop::Color::reset;
	print_dep(get_implicit_dependencies());
	esd.output << indent << prop::Color::static_text << "           Dependents: " << prop::Color::reset;
	print_dep(get_dependents());
}

void prop::detail::Property_base::print_status(const Extended_status_data &esd) const {
	print_extended_status(esd, 0);
}
