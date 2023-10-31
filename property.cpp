#include "property.h"

#include <cassert>
#include <stdexcept>

#ifdef PROPERTY_DEBUG
#include <iostream>
#define TRACE(...) std::clog << __FILE__ << ' ' << __PRETTY_FUNCTION__ << ':' << __LINE__ << " " << __VA_ARGS__ << '\n'
static int property_base_counter;
#else
#define TRACE(...)
#endif

void pro::detail::Property_base::update() {
	if (need_update) {
		need_update = false;
		update();
	}
}

void pro::detail::Property_base::read_notify() {
	if (current_binding) {
		current_binding->dependencies.add(this);
		dependents.add(current_binding);
	}
}

void pro::detail::Property_base::read_notify() const {
	const_cast<pro::detail::Property_base *>(this)->read_notify();
}

void pro::detail::Property_base::write_notify() {
	for (const auto &dependent : dependents) {
		dependent->need_update = true;
	}
	for (const auto &dependent : dependents) {
		dependent->Property_base::update();
	}
}

void pro::detail::Property_base::update_start() {
	TRACE(this << " " << name);
	previous_binding = current_binding;
	clear_dependencies();
	current_binding = this;
}

void pro::detail::Property_base::update_complete() {
	TRACE(this << " " << name);
	current_binding = previous_binding;
}

pro::detail::Property_base::Property_base()
#ifdef PROPERTY_DEBUG
	: name("p" + std::to_string(++property_base_counter))
#endif
{
	TRACE(this << " " << name << " with creation binding " << creation_binding << " "
			   << (creation_binding ? creation_binding->name : ""));
}

pro::detail::Property_base::~Property_base() {
	TRACE(this << " " << name);
	clear_dependencies();
	if (dependents.has(creation_binding)) {
		dependents.remove(creation_binding);
		creation_binding->dependencies.remove(this);
	}
	assert(dependents.count() == 0);
	if (dependents.count() != 0) {
		TRACE(dependents.count());
		for (const auto &d : dependents) {
			TRACE(this << " " << name << " depends on " << d << " " << d->name);
		}
		throw std::runtime_error{"Dangling reference"};
	}
}

void pro::detail::Property_base::clear_dependencies() {
	for (const auto &dependency : dependencies) {
		dependency->dependents.remove(this);
	}
	dependencies.clear();
}

bool pro::detail::Binding_set::has(Property_base *pb) const {
	return set.contains(pb);
}

std::size_t pro::detail::Binding_set::count() const {
	return set.size();
}

void pro::detail::Binding_set::add(Property_base *pb) {
	set.insert(pb);
}

void pro::detail::Binding_set::remove(Property_base *pb) {
	set.erase(pb);
}

void pro::detail::Binding_set::clear() {
	set.clear();
}

std::set<pro::detail::Property_base *>::iterator pro::detail::Binding_set::begin() {
	return std::begin(set);
}

std::set<pro::detail::Property_base *>::iterator pro::detail::Binding_set::end() {
	return std::end(set);
}

std::set<pro::detail::Property_base *>::const_iterator pro::detail::Binding_set::begin() const {
	return std::begin(set);
}

std::set<pro::detail::Property_base *>::const_iterator pro::detail::Binding_set::end() const {
	return std::end(set);
}

std::set<pro::detail::Property_base *>::const_iterator pro::detail::Binding_set::cbegin() const {
	return std::cbegin(set);
}

std::set<pro::detail::Property_base *>::const_iterator pro::detail::Binding_set::cend() const {
	return std::cend(set);
}
