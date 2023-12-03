#include "property.h"

#include <cassert>
#include <stdexcept>

#ifdef PROPERTY_DEBUG
#include <iostream>
#define TRACE(...) std::clog << __FILE__ << ':' << __LINE__ << ' ' << __PRETTY_FUNCTION__ << "\n" << __VA_ARGS__ << '\n'
int property_base_counter;
static std::string propnames(const prop::detail::Binding_set &set) {
	std::string result{'['};
	const char *separator = "";
	for (const auto &binding : set) {
		result += separator;
		separator = ", ";
		result += binding->name;
	}
	result.push_back(']');
	return result;
}
#else
#define TRACE(...)
#endif

void (*prop::on_property_severed)(prop::detail::Property_base *severed, prop::detail::Property_base *reason) =
	[](prop::detail::Property_base *, prop::detail::Property_base *) {};

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
	for (auto dep_it = std::begin(dependents); dep_it != std::end(dependents);) {
		auto &dependent = *dep_it;
		++dep_it;
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
		TRACE("Added " << name << " as an implicit dependency of " << current_binding->name);
		current_binding->implicit_dependencies.add(this);
	}
}

void prop::detail::Property_base::read_notify() const {
	const_cast<prop::detail::Property_base *>(this)->read_notify();
}

void prop::detail::Property_base::write_notify() {
	TRACE(name << "->" << propnames(dependents));
	for (const auto &dependent : dependents) {
		dependent->need_update = true;
	}
	for (auto it = std::begin(dependents); it != std::end(dependents);) {
		auto &dependent = *it;
		++it;
		dependent->Property_base::update();
	}
}

void prop::detail::Property_base::update_start() {
	TRACE(name);
	previous_binding = current_binding;
	clear_implicit_dependencies();
	current_binding = this;
}

void prop::detail::Property_base::update_complete() {
	TRACE(name);
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

#ifdef PROPERTY_DEBUG
prop::detail::Property_base::Property_base(std::string_view type_name)
	: name("p" + std::to_string(++property_base_counter) + "<" + std::string{type_name} + ">")
#else
prop::detail::Property_base::Property_base()
#endif
{
	TRACE("Created " << name << (creation_binding ? " with creation binding " + creation_binding->name : ""));
}

prop::detail::Property_base::Property_base(Property_base &&other) {
	TRACE(other.name << " split into "
					 << "<" + other.name << " (moved from) and "
					 << ">" + other.name << " (moved to)");
#ifdef PROPERTY_DEBUG
	name = ">" + other.name;
	other.name = "<" + other.name;
#endif
	using std::swap;
	swap(explicit_dependencies, other.explicit_dependencies);
	swap(implicit_dependencies, other.implicit_dependencies);
	swap(dependents, other.dependents);

	for (auto dependent : dependents) {
		dependent->explicit_dependencies.replace(&other, this);
		dependent->implicit_dependencies.replace(&other, this);
	}
}

void prop::detail::Property_base::operator=(Property_base &&other) {
	TRACE(other.name << " was moved to " << name);
	sever_implicit_dependents();
	other.sever_implicit_dependents();
	take_explicit_dependents(std::move(other));
}

prop::detail::Property_base::~Property_base() {
	TRACE("Destroying " << name);
	//clear implicit dependencies
	clear_implicit_dependencies();
	//clear explicit dependencies
	for (const auto &dependency : explicit_dependencies) {
		if (dependency) {
			TRACE("Removed " << name << " from dependents of " << dependency->name);
			dependency->dependents.remove(this);
		}
	}
	//clear creation dependency
	if (dependents.has(creation_binding)) {
		TRACE("Removed " << name << " as dependency from " << creation_binding->name << " because " << name
						 << " is a temporary getting destroyed");
		dependents.remove(creation_binding);
		creation_binding->implicit_dependencies.remove(this);
	}
	//remove dependents
	sever_implicit_dependents();
	for (auto &dependent : dependents) {
		for (auto &explicit_dependency : dependent->explicit_dependencies) {
			if (explicit_dependency == this) {
#ifdef PROPERTY_DEBUG
				TRACE("Explicit dependency " << name << " of " << dependent->name << " is no longer available");
#endif
				explicit_dependency = nullptr;
			}
		}
		dependent->update();
	}
	TRACE(name << " destroyed");
#ifdef PROPERTY_DEBUG
	name = "~" + name;
#endif
}

void prop::detail::Property_base::clear_implicit_dependencies() {
	if (implicit_dependencies.is_empty()) {
		return;
	}
	TRACE(name);
	for (const auto &dependency : implicit_dependencies) {
		TRACE("Removed " << name << " from dependents of " << dependency->name);
		assert(dependency->dependents.has(this));
		dependency->dependents.remove(this);
	}
	implicit_dependencies.clear();
}

void prop::detail::Property_base::sever_implicit_dependents() {
	for (auto dep_it = std::cbegin(dependents); dep_it != std::cend(dependents);) {
		auto &dependent = *dep_it;
		++dep_it;
		//unbind dependents
		if (dependent->implicit_dependencies.has(this)) {
			if (dependent->explicit_dependencies.has(this)) {
				dependent->implicit_dependencies.remove(this);
			} else {
				TRACE("Unbound " << dependent->name << " because it implicitly depends on " << name
								 << " which is getting severed");
				on_property_severed(dependent, this);
				dependent->unbind();
			}
		}
	}
}

void prop::detail::Property_base::take_explicit_dependents(Property_base &&source) {
	for (auto it = std::begin(source.dependents); it != std::end(source.dependents);) {
		auto &source_dependent = *it;
		++it;
		if (source_dependent->explicit_dependencies.has(&source)) {
			TRACE(name << " adopting explicit dependent " << source_dependent->name << " from " << source.name);
			dependents.add(source_dependent);
			for (auto &explicit_dependent : source_dependent->explicit_dependencies) {
				if (explicit_dependent == &source) {
					explicit_dependent = this;
				}
			}
			if (!source_dependent->implicit_dependencies.has(&source)) {
				source.dependents.remove(source_dependent);
			}
		}
	}
}

void prop::detail::swap(Property_base &lhs, Property_base &rhs) {
	TRACE("Swapping " << lhs.name << " and " << rhs.name);
	using std::swap;
	swap(lhs.explicit_dependencies, rhs.explicit_dependencies);
	swap(lhs.implicit_dependencies, rhs.implicit_dependencies);
	swap(lhs.dependents, rhs.dependents);

	for (auto dependent : lhs.dependents) {
		dependent->explicit_dependencies.replace(&rhs, &lhs);
		dependent->implicit_dependencies.replace(&rhs, &lhs);
	}
	for (auto dependent : rhs.dependents) {
		dependent->explicit_dependencies.replace(&lhs, &rhs);
		dependent->implicit_dependencies.replace(&lhs, &rhs);
	}
}

bool prop::detail::Binding_set::has(Property_base *pb) const {
	return set.contains(pb);
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

std::set<prop::detail::Property_base *>::iterator prop::detail::Binding_set::begin() {
	return std::begin(set);
}

std::set<prop::detail::Property_base *>::iterator prop::detail::Binding_set::end() {
	return std::end(set);
}

std::set<prop::detail::Property_base *>::const_iterator prop::detail::Binding_set::begin() const {
	return std::begin(set);
}

std::set<prop::detail::Property_base *>::const_iterator prop::detail::Binding_set::end() const {
	return std::end(set);
}

std::set<prop::detail::Property_base *>::const_iterator prop::detail::Binding_set::cbegin() const {
	return std::cbegin(set);
}

std::set<prop::detail::Property_base *>::const_iterator prop::detail::Binding_set::cend() const {
	return std::cend(set);
}

bool prop::detail::Binding_list::has(Property_base *property) const {
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

prop::Property<void>::Property(std::function<void()> f)
#ifdef PROPERTY_DEBUG
	: Property_base{"void"}
#endif
{
	update_source([source = std::move(f)](const prop::detail::Binding_list &) { source(); });
}

void prop::Property<void>::update() {
	if (!source) {
		return;
	}
	detail::RAII updater{[this] { update_start(); },
						 [this] {
							 update_complete();
							 need_update = false;
						 }};
	try {
		source(explicit_dependencies);
	} catch (const prop::Property_expired &) {
		unbind();
	}
}

void prop::Property<void>::unbind() {
	source = nullptr;
	Property_base::unbind();
}

void prop::Property<void>::update_source(std::function<void(const prop::detail::Binding_list &)> f) {
	std::swap(f, source);
	update();
}
