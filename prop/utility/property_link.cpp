#include "property_link.h"
#include "color.h"
#include "tracking_list.h"
#include "type_name.h"
#include "utility.h"

#ifdef PROP_LIFETIMES
std::map<const prop::Property_link *, prop::Property_link::Property_link_lifetime_status> &
prop::Property_link::lifetimes() {
	static std::map<const prop::Property_link *, prop::Property_link::Property_link_lifetime_status>
		Property_link_lifetimes;
	return Property_link_lifetimes;
}
#endif

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
	Tracer(std::ostream &output_stream = std::cout, std::source_location sl = std::source_location::current())
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
		os << prop::color_address(p);
		return std::move(*this);
	}
	Tracer &&operator<<(std::span<const prop::Property_link::Property_pointer> span) && {
		os << '[';
		const char *sep = "";
		for (auto &l : span) {
			os << sep << (l.is_required() ? "!" : "") << l->to_string() << prop::Color::static_text << "{"
			   << prop::Color::variable_value << l->value_string() << prop::Color::static_text << "}";
			sep = ", ";
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

void prop::Property_link::read_notify() const {
	assert_status();
	binding_data.read_notify(this);
}

void prop::Property_link::write_notify() {
	assert_status();
	if (explicit_dependencies + implicit_dependencies == dependencies.size()) {
		return;
	}
	TRACE("Notifying  " << get_name() << "->" << get_dependents());
	switch (dependencies.size() - explicit_dependencies - implicit_dependencies) {
		case 0:
			return;
		case 1:
			dependencies.back()->Property_link::update();
			break;
		default:
			for (auto &dependent : prop::Tracking_list<>::of_dependents(*this)) {
				dependent->Property_link::update();
			}
			break;
	}
}

const prop::Update_data prop::Property_link::update_start() {
	assert_status();
	return binding_data.update_start(this);
}

void prop::Property_link::update_complete(const prop::Update_data &update_data) {
	binding_data.update_end(update_data);
}

std::string prop::Property_link::to_string() const {
	return to_string(type());
}

std::string prop::Property_link::to_string(std::string_view type_name) const {
	std::stringstream ss;
	ss << prop::Color::type;
	ss << prop::color_type(std::string{type_name});
	ss << prop::Color::static_text << '@' << prop::color_address(this);
	if (custom_name.empty()) {
		ss << prop::Color::static_text;
	} else {
		ss << prop::Color::variable_name << ' ' << custom_name << prop::Color::static_text;
	}
	return ss.str();
}

prop::Property_link::Property_link()
	: Property_link(prop::type_name<std::remove_cvref_t<decltype(*this)>>()) {}

prop::Property_link::Property_link([[maybe_unused]] std::string_view type) {
	set_status();
	if (binding_data.current_binding()) {
		TRACE("Created    " << to_string(type) << " inside binding of\n           "
							<< binding_data.current_binding()->to_string());
		binding_data.read_notify({this, false});
	} else {
		TRACE("Created    " << to_string(type));
	}
}

prop::Property_link::Property_link(std::vector<prop::Property_link::Property_pointer> initial_explicit_dependencies)
	: dependencies{std::move(initial_explicit_dependencies)}
	, explicit_dependencies{static_cast<decltype(explicit_dependencies)>(dependencies.size())} {
	set_status();
	for (auto &explicit_dependency : dependencies) {
		if (auto ptr = explicit_dependency.get_pointer()) {
			ptr->add_dependent(*this);
		}
	}
	if (binding_data.current_binding()) {
		TRACE("Created    " << to_string(type()) << " inside binding of\n           "
							<< binding_data.current_binding()->to_string());
		binding_data.read_notify({this, false});
	} else {
		TRACE("Created    " << to_string(type()));
	}
}

prop::Property_link::Property_link(Property_link &&other) {
	set_status();
	TRACE("Moved " << other.get_name() << " from  " << &other << " to " << to_string());
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

void prop::Property_link::unbind() {
	assert_status();
	TRACE("Unbinding  " << get_status());
	for (std::size_t i = 0; i < explicit_dependencies + implicit_dependencies; i++) {
		if (dependencies[i]) {
			TRACE("Removing   " << dependencies[i]->to_string() << " from dependencies of " << to_string());
			dependencies[i]->remove_dependent(*this);
		}
	}
	dependencies.erase(std::begin(dependencies),
					   std::begin(dependencies) + explicit_dependencies + implicit_dependencies);
	explicit_dependencies = implicit_dependencies = 0;
}

void prop::Property_link::operator=(Property_link &&other) {
	assert_status();
	other.assert_status();
	TRACE("Moving     " << other.get_name() << " to " << get_name());
	swap(*this, other);
}

prop::Property_link::~Property_link() {
	assert_status();
	TRACE("Destroying " << get_status());
	binding_data.remove(this);
	for (std::size_t dependency_index = 0; dependency_index < explicit_dependencies + implicit_dependencies;
		 dependency_index++) {
		auto &dependency = dependencies[dependency_index];
		if (dependency) {
			TRACE("Removing   " << to_string() << " from dependents of " << dependency->to_string());
			dependency->remove_dependent(*this);
		}
	}
	for (std::size_t dependent_index = explicit_dependencies + implicit_dependencies;
		 dependent_index < std::size(dependencies); ++dependent_index) {
		auto &dependent = *dependencies[dependent_index];
		TRACE("Handling dependent " << dependent.get_status());
		bool update_needed = false;
		for (std::uint16_t dependent_dependency_index = 0;
			 dependent_dependency_index < dependent.explicit_dependencies + dependent.implicit_dependencies;
			 ++dependent_dependency_index) {
			auto &dependent_dependency = dependent.dependencies[dependent_dependency_index];
			if (dependent_dependency != this) {
				continue;
			}
			if (dependent_dependency.is_required()) {
				TRACE("Removing   " << to_string() << " from required dependencies of " << dependent.get_name());
				dependent.unbind();
				dependent_index--;
				update_needed = false;
				break; //all dependents and dependencies are gone
			}
			if (dependent_dependency_index < dependent.explicit_dependencies) { //explicit dependency
				dependent_dependency = nullptr;
				TRACE("Removed    " << to_string() << " from optional explicit dependencies of "
									<< dependent.get_name());
				update_needed = true;
				continue; //duplicate explicit dependency possible
			} else {	  //implicit dependency
				dependent.dependencies.erase(std::begin(dependent.dependencies) + dependent_dependency_index);
				dependent.implicit_dependencies--;
				TRACE("Removed    " << to_string() << " from optional implicit dependencies of "
									<< dependent.get_name());
				update_needed = true;
				break; //only 1 implicit dependency possible
			}
		}
		if (update_needed and &dependent != binding_data.current_binding()) {
			dependent.update();
		}
	}
	TRACE("Destroyed  " << to_string());
#ifdef PROPERTY_DEBUG
	custom_name = "~" + custom_name;
#endif
	set_status(Property_link_lifetime_status::post);
}

void prop::swap(Property_link &lhs, Property_link &rhs) {
	//TODO: Surely there is a simpler way to do this
	lhs.assert_status();
	rhs.assert_status();
	TRACE("Swapping   " << lhs.get_name() << " and " << rhs.get_name());
	if (lhs.dependencies.empty() and rhs.dependencies.empty()) {
		return;
	}
	struct Tmp final : Property_link {
		Tmp()
			: Property_link(prop::type_name<Tmp>()) {}
	} static sentinel;

	auto replace = [](Property_link &pb_old, Property_link &pb_new) {
		std::swap(pb_old.custom_name, pb_new.custom_name);
		assert(pb_new.dependencies.empty());
		assert(pb_new.explicit_dependencies == 0);
		assert(pb_new.implicit_dependencies == 0);
		assert(pb_old.explicit_dependencies + pb_old.implicit_dependencies <= pb_old.dependencies.size());
		if (pb_old.dependencies.empty()) {
			return;
		}
		while (pb_new.dependencies.size() < pb_old.explicit_dependencies + pb_old.implicit_dependencies) {
			auto &dependency = pb_old.dependencies[pb_new.dependencies.size()];
			dependency->replace_dependent(pb_old, pb_new);
			pb_new.dependencies.push_back(dependency);
		}
		pb_new.explicit_dependencies = pb_old.explicit_dependencies;
		pb_new.implicit_dependencies = pb_old.implicit_dependencies;
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
		pb_old.explicit_dependencies = pb_old.implicit_dependencies = 0;
		pb_old.dependencies.clear();
	};
	replace(lhs, sentinel);
	replace(rhs, lhs);
	replace(sentinel, rhs);
}

std::string_view prop::Property_link::type() const {
	return prop::type_name<std::remove_cvref_t<decltype(*this)>>();
}

std::string prop::Property_link::value_string() const {
	return "";
}

bool prop::Property_link::has_source() const {
	return false;
}

void prop::Property_link::set_explicit_dependencies(std::vector<Property_link::Property_pointer> &&deps) {
	assert_status();
	assert(deps.size() < std::numeric_limits<decltype(explicit_dependencies)>::max());
	if (dependencies.empty()) {
		TRACE("Setting    " << to_string() << "'s explicit dependencies to\n           " << deps);
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
		TRACE("Removing   " << to_string() << "'s explicit dependencies " << dependencies);
		for (std::size_t i = 0; i < explicit_dependencies; i++) {
			dependencies[i]->remove_dependent(*this);
		}
		dependencies.erase(std::begin(dependencies), std::begin(dependencies) + explicit_dependencies);
		explicit_dependencies = 0;
		return;
	}
	TRACE("Replacing  " << to_string() << "'s explicit dependencies:\n           old: " << dependencies
						<< "\n           new: " << deps);
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

void prop::Property_link::print_extended_status(const prop::Extended_status_data &esd, int current_depth) const {
	assert_status();
	std::string indent;
	for (int i = 0; i < current_depth; i++) {
		indent += esd.indent_with;
	}
	auto print_dep = [&esd, &indent, current_depth](std::span<const prop::Property_link::Property_pointer> deps) {
		if (deps.empty()) {
			esd.output << "[]\n";
		} else {
			esd.output << prop::Color::reset << "[";
			const char *sep = "";
			if (esd.depth == current_depth) {
				for (auto &dep : deps) {
					esd.output << prop::Color::static_text << sep;
					if (dep) {
						if (dep.is_required()) {
							esd.output << prop::Color::white << '!';
						}
						esd.output << dep->to_string();
						esd.output << prop::Color::static_text << "{" << prop::Color::variable_value
								   << dep->displayed_value() << prop::Color::static_text << "}";
					} else {
						esd.output << prop::Color::address_highlight << "nullptr";
					}
					sep = ", ";
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
	esd.output << to_string() << '\n';
	esd.output << indent << prop::Color::static_text << "           Value: " << prop::Color::variable_value
			   << value_string() << "\n";
	esd.output << indent << prop::Color::static_text << "           Bound: " << prop::Color::variable_value
			   << (has_source() ? "Yes" : "No") << "\n";
	esd.output << indent << prop::Color::static_text
			   << std::format("{:12} explicit dependencies: ", explicit_dependencies) << prop::Color::reset;
	print_dep(get_explicit_dependencies());
	esd.output << indent << prop::Color::static_text
			   << std::format("{:12} implicit dependencies: ", implicit_dependencies) << prop::Color::reset;
	print_dep(get_implicit_dependencies());
	esd.output << indent << prop::Color::static_text
			   << std::format("{:12} dependents: ", dependencies.size() - explicit_dependencies - implicit_dependencies)
			   << prop::Color::reset;
	print_dep(get_dependents());
}

void prop::Property_link::print_status(const Extended_status_data &esd) const {
	assert_status();
	print_extended_status(esd, 0);
}

std::string prop::Property_link::get_status() const {
	std::stringstream ss;
	print_status({.output = ss});
	return std::move(ss).str();
}

static std::size_t current_index;

void prop::Implicit_dependency_list::read_notify(const Property_link *p) {
	read_notify({p, true});
}

void prop::Implicit_dependency_list::read_notify(prop::Required_pointer<Property_link> p) {
	if (data.empty()) {
		return;
	}
	const auto current = current_binding();
	if (p == current) {
		return;
	}
	const auto is_an_explicit_dependency = [&] {
		return std::find(std::begin(current->dependencies),
						 std::begin(current->dependencies) + current->explicit_dependencies,
						 p) != std::begin(current->dependencies) + current->explicit_dependencies;
	};
	const auto is_duplicate_dependency = [&] {
		return std::find(std::begin(data) + current_index, std::end(data), p) != std::end(data);
	};
	if (not is_an_explicit_dependency() and not is_duplicate_dependency()) {
		data.push_back(p);
		TRACE("Added      " << p->get_name() << " as an implicit dependency of\n           " << current->get_name());
	}
}

const prop::Update_data prop::Implicit_dependency_list::update_start(Property_link *p) {
	TRACE("Updating   " << p->get_status());
	data.push_back({p, false});
	current_index = data.size();
	return {.index = current_index - 1};
}

void prop::Implicit_dependency_list::update_end(const prop::Update_data &update_data) {
	const auto current = current_binding();
	assert(current);

	const auto new_implicit_dependencies = std::size(data) - current_index;

	//overwrite overlap
	const auto overlap_size = std::min<std::size_t>(current->implicit_dependencies, new_implicit_dependencies);
	for (std::size_t i = 0; i < overlap_size; i++) {
		auto &old = current->dependencies[i + current->explicit_dependencies];
		auto &now = data[current_index + i];
		if (old.get_pointer() != now.get_pointer()) {
			old->remove_dependent(*current);
			old = now;
			old->add_dependent(*current);
		}
		old.set_required(now.is_required());
	}

	if (new_implicit_dependencies > current->implicit_dependencies) {
		//insert additional dependencies
		for (auto it = std::begin(data) + current_index + current->implicit_dependencies; it != std::end(data); ++it) {
			it->get_pointer()->add_dependent(*current);
		}
		current->dependencies.insert(std::begin(current->dependencies) + current->explicit_dependencies +
										 current->implicit_dependencies,
									 std::begin(data) + current_index + current->implicit_dependencies, std::end(data));
	} else if (new_implicit_dependencies < current->implicit_dependencies) {
		//remove extra dependencies
		const auto from =
			std::begin(current->dependencies) + current->explicit_dependencies + new_implicit_dependencies;
		const auto to =
			std::begin(current->dependencies) + current->explicit_dependencies + current->implicit_dependencies;
		for (auto it = from; it < to; ++it) {
			it->get_pointer()->remove_dependent(*current);
		}
		current->dependencies.erase(from, to);
	}
	current->implicit_dependencies = new_implicit_dependencies;
	data.resize(current_index - 1, {nullptr, false});
	current_index = update_data.index;
	TRACE("Updated    " << current->get_status());
}

prop::Property_link *prop::Implicit_dependency_list::current_binding() const {
	if (data.empty()) {
		return nullptr;
	}
	return data[current_index - 1];
}

void prop::Implicit_dependency_list::remove(Property_link *p) {
	if (data.empty()) {
		return;
	}
	for (std::size_t i = current_index; i < std::size(data); ++i) {
		if (data[i] == p) {
			data.erase(std::begin(data) + i);
			TRACE("Removed    " << p->to_string() << " from dependents of " << data[current_index - 1]->get_name());
			return;
		}
	}
}
