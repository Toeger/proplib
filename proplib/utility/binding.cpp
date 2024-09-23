#include "binding.h"

#include <cassert>
#if 0
prop::Binding::Binding()
	:
#ifdef PROPERTY_DEBUG
	Property_base{"void"}
	,
#endif
	callback{[] {}} {
}

prop::Binding::Binding(std::function<void()> callback)
#ifdef PROPERTY_DEBUG
	: Property_base{"void"}
#endif
{
	update_source(std::move(callback));
}

prop::Binding &prop::Binding::operator=(std::function<void()> callback) {
	update_source(std::move(callback));
	return *this;
}

void prop::Binding::unbind() {
	callback = nullptr;
	Property_base::unbind();
}

void prop::Binding::update() {
	detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
	callback();
}

void prop::Binding::update_source(std::function<void()> f) {
	callback = std::move(f);
	update();
}
#endif
