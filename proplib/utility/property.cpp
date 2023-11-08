#include "property.h"

#include <cassert>
#include <stdexcept>

#ifdef PROPERTY_DEBUG
#include <iostream>
#define TRACE(...) std::clog << __FILE__ << ' ' << __PRETTY_FUNCTION__ << ':' << __LINE__ << " " << __VA_ARGS__ << '\n'
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
			for (auto &explicit_dependency : dependent->explicit_depenencies) {
				if (explicit_dependency == this) {
					explicit_dependency = nullptr;
				}
			}
		}
	}
}

void prop::detail::Property_base::read_notify() {
	if (current_binding && !dependents.add(current_binding)) {
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
	for (const auto &dependent : dependents) {
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

prop::detail::Property_base::Property_base()
#ifdef PROPERTY_DEBUG
	: name("p" + std::to_string(++property_base_counter))
#endif
{
	TRACE("Created " << name << (creation_binding ? " with creation binding " + creation_binding->name : ""));
}

prop::detail::Property_base::~Property_base() noexcept(false) {
	//clear implicit dependencies
	clear_implicit_dependencies();
	//clear explicit dependencies
	for (const auto &dependency : implicit_dependencies) {
		dependency->dependents.remove(this);
	}
	//clear creation dependency
	implicit_dependencies.clear();
	if (dependents.has(creation_binding)) {
		TRACE("Removed " << name << " as dependency from " << creation_binding->name << " because " << name
						 << " is a temporary getting destroyed");
		dependents.remove(creation_binding);
		creation_binding->implicit_dependencies.remove(this);
	}
	//remove dependents
	for (auto dep_it = std::begin(dependents); dep_it != std::end(dependents);) {
		auto &dependent = *dep_it;
		//unbind dependents
		sever_implicit_dependents();
		for (auto &explicit_dependency : dependent->explicit_depenencies) {
#ifdef PROPERTY_DEBUG
			if (explicit_dependency) {
				TRACE("Explicit dependency " << name << " of " << explicit_dependency->name
											 << " is no longer available");
			}
#endif
			explicit_dependency = nullptr;
		}
		dep_it = dependents.set.erase(dep_it);
	}
	TRACE(name << " destroyed");
}

void prop::detail::Property_base::clear_implicit_dependencies() {
	if (implicit_dependencies.is_empty()) {
		return;
	}
	TRACE(name);
	for (const auto &dependency : implicit_dependencies) {
		dependency->dependents.remove(this);
	}
	implicit_dependencies.clear();
}

void prop::detail::Property_base::sever_implicit_dependents() {
	for (auto dep_it = std::begin(dependents); dep_it != std::end(dependents);) {
		auto &dependent = *dep_it;
		++dep_it;
		//unbind dependents
		if (dependent->implicit_dependencies.has(this)) {
			TRACE("Unbound " << dependent->name << " because it implicitly depends on " << name
							 << " which is getting severed");
			on_property_severed(dependent, this);
			dependent->unbind();
		}
	}
}

void prop::detail::Property_base::take_explicit_dependents(Property_base &&source) {
	for (auto &dependent : source.dependents) {
		if (!dependent->implicit_dependencies.has(&source)) {
			TRACE(name << " adopting explicit dependent " << dependent << " from " << source.name);
			for (auto &explicit_dependent : dependent->explicit_depenencies) {
				if (explicit_dependent == &source) {
					explicit_dependent = this;
					dependents.add(explicit_dependent);
					source.dependents.remove(explicit_dependent);
				}
			}
		}
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
