#include "binding.h"

#include <cassert>

prop::Property<void>::Property(std::function<void()> source) {
	update_source(std::move(source));
}

prop::Property<void> &prop::Property<void>::operator=(std::function<void()> source) {
	std::swap(source, this->source);
	return *this;
}

void prop::Property<void>::unbind() {
	source = nullptr;
	clear_dependencies();
}

void prop::Property<void>::update() {
	{
		detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
		source();
	}
	assert(dependents.is_empty());
}

void prop::Property<void>::update_source(std::function<void()> f) {
	source = std::move(f);
	update();
}
