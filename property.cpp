#include "property.h"

void pro::detail::Property_base::update() {
	if (need_update) {
		need_update = false;
		update();
	}
}

void pro::detail::Property_base::read_notify() const {
	read_notify(const_cast<Property_base *>(this));
}

void pro::detail::Property_base::read_notify(Property_base *base) {
	if (current_binding) {
		current_binding->push_back(base);
	}
}

void pro::detail::Property_base::write_notify() {
	for (const auto &dependent : dependents) {
		dependent->need_update = true;
		dependent->Property_base::update();
	}
}

void pro::detail::Property_base::unbind() {
	for (const auto &dependency : dependencies) {
		dependency->dependents.erase(
			std::find(std::begin(dependency->dependents), std::end(dependency->dependents), this));
	}
}

void pro::detail::Property_base::update_start() {
	previous_binding = current_binding;
	current_binding = &dependencies;
}

void pro::detail::Property_base::update_complete() {
	current_binding = previous_binding;
	for (const auto &dependency : dependencies) {
		dependency->dependents.push_back(this);
	}
}
