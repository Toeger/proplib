#include "property.h"

void (*prop::on_property_severed)(prop::detail::Property_base *severed, prop::detail::Property_base *reason) =
	[](prop::detail::Property_base *, prop::detail::Property_base *) {};

void (*prop::on_property_update_exception)(std::exception_ptr exception) = [](std::exception_ptr) {};

prop::Property<void>::Property() {}

prop::Property<void> &prop::Property<void>::operator=(prop::detail::Property_function_binder<void> binder) {
	Property_base::set_explicit_dependencies(std::move(binder.explicit_dependencies));
	update_source(std::move(binder.function));
	return *this;
}

void prop::Property<void>::update() {
	if (!source) {
		return;
	}
	prop::detail::RAII updater{[this] { update_start(); },
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

std::string prop::detail::Property_base::displayed_value() const {
	return {};
}

void prop::Property<void>::unbind() {
	source = nullptr;
	Property_base::unbind();
}

void prop::Property<void>::update_source(std::move_only_function<void(const prop::detail::Binding_list &)> f) {
	std::swap(f, source);
	update();
}

bool prop::Property<void>::is_bound() const {
	return !!source;
}

void prop::print_status(const prop::Property<void> &p, std::ostream &os) {
	const auto &static_text_color = prop::Color::static_text;
	os << static_text_color << "Property   " << prop::Color::reset;
#ifdef PROPERTY_NAMES
	os << p.get_name();
#else
	os << &p;
#endif
	os << '\n';
	os << static_text_color << "\tsource: " << prop::Color::reset << (p.source ? "Yes" : "No") << "\n";
	os << static_text_color << "\tExplicit dependencies: [" << prop::Color::reset << p.explicit_dependencies
	   << static_text_color << "]\n";
	os << static_text_color << "\tImplicit dependencies: [" << prop::Color::reset << p.get_implicit_dependencies()
	   << static_text_color << "]\n";
	os << static_text_color << "\tDependents: [" << prop::Color::reset << p.get_dependents() << static_text_color
	   << "]\n";
}

prop::Property<void>::Property(prop::Property<void> &&other) {
	std::swap(source, other.source);
}

prop::Property<void> &prop::Property<void>::operator=(prop::Property<void> &&other) {
	std::swap(source, other.source);
	return *this;
}
