#include "property.h"

prop::Property<void>::Property() {}

prop::Property<void> &prop::Property<void>::operator=(prop::detail::Property_function_binder<void> binder) {
	Property_link::set_explicit_dependencies(std::move(binder.dependencies));
	update_source(std::move(binder.function));
	return *this;
}

void prop::Property<void>::update() {
	if (!source) {
		return;
	}
	auto data = update_start();
	prop::detail::RAII updater{[this, &data] { update_complete(data); }};
	switch (source(get_explicit_dependencies())) {
		case prop::Updater_result::unchanged:
			return;
		case prop::Updater_result::changed:
			write_notify();
			return;
		case prop::Updater_result::unbind: {
			auto{std::move(source)};
		}
	}
}

void prop::Property<void>::unbind() {
	source = nullptr;
	Property_link::unbind();
}

void prop::Property<void>::update_source(
	std::move_only_function<prop::Updater_result(std::span<const prop::Property_link::Property_pointer>)> f) {
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
