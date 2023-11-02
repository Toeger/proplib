#include "property.h"

#include <cassert>
#include <stdexcept>

#ifdef PROPERTY_DEBUG
#include <iostream>
#define TRACE(...) std::clog << __FILE__ << ' ' << __PRETTY_FUNCTION__ << ':' << __LINE__ << " " << __VA_ARGS__ << '\n'
static int property_base_counter;
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

void prop::detail::Property_base::update() {
	if (need_update) {
		need_update = false;
		update();
	}
}

void prop::detail::Property_base::read_notify() {
	if (current_binding && !dependents.has(current_binding)) {
		TRACE("Adding " << name << " as a dependency of " << current_binding->name);
		current_binding->dependencies.add(this);
		dependents.add(current_binding);
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
	clear_dependencies();
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
	clear_dependencies();
	if (dependents.has(creation_binding)) {
		TRACE("Removed " << name << " as dependency from " << creation_binding->name << " because " << name
						 << " is getting destroyed");
		dependents.remove(creation_binding);
		creation_binding->dependencies.remove(this);
	}
	assert(dependents.count() == 0);
	if (dependents.count() != 0) {
		TRACE(name << " is being destroyed but still has dependencies " << propnames(dependents));
		throw std::runtime_error{"Dangling reference"};
	}
	TRACE(name << " destroyed");
}

void prop::detail::Property_base::clear_dependencies() {
	if (dependencies.count() == 0) {
		return;
	}
	TRACE(name);
	for (const auto &dependency : dependencies) {
		dependency->dependents.remove(this);
	}
	dependencies.clear();
}

bool prop::detail::Binding_set::has(Property_base *pb) const {
	return set.contains(pb);
}

std::size_t prop::detail::Binding_set::count() const {
	return set.size();
}

void prop::detail::Binding_set::add(Property_base *pb) {
	set.insert(pb);
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
