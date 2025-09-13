#include "property.h"

prop::Property<void>::Property() {}

prop::Property<void> &prop::Property<void>::operator=(prop::detail::Property_function_binder<void> binder) {
	Property_base::set_explicit_dependencies(std::move(binder.dependencies));
	update_source(std::move(binder.function));
	return *this;
}

void prop::Property<void>::update() {
	if (!source) {
		return;
	}
	Property_base *previous_binding;
	prop::detail::RAII updater{[this, &previous_binding] { update_start(previous_binding); },
							   [this, &previous_binding] { update_complete(previous_binding); }};
	source(get_explicit_dependencies());
}

void prop::Property<void>::unbind() {
	source = nullptr;
	Property_base::unbind();
}

void prop::Property<void>::update_source(
	std::move_only_function<void(std::span<const prop::detail::Property_link>)> f) {
	std::swap(f, source);
	update();
}

bool prop::Property<void>::is_bound() const {
	return !!source;
}

prop::Property<void>::Property(prop::Property<void> &&other) {
	std::swap(source, other.source);
}

prop::Property<void> &prop::Property<void>::operator=(prop::Property<void> &&other) {
	std::swap(source, other.source);
	return *this;
}
