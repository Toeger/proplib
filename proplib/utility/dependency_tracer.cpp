#include "dependency_tracer.h"
#include "color.h"

void prop::Dependency_tracer::print_widget_trace(std::ostream &os) const {
	for (auto &[address, common_data] : widgets) {
		os << prop::Color::variable_name << common_data.name << prop::Color::static_text << '@' << prop::Color::address
		   << address << '\n';
		for (auto &sub_data : common_data.data) {
			os << prop::Color::static_text << "as " << prop::Color::type << sub_data.type << prop::Color::static_text
			   << ":\n";
			for (auto &property : sub_data.properties) {
				auto &property_data = properties.find(property)->second;
				os << '\t' << prop::Color::type << property_data.type << ' ' << prop::Color::variable_name
				   << property_data.name << prop::Color::static_text << '@' << prop::Color::address << property << "\n";
			}
		}
	}
	os << prop::Color::reset;
}

void prop::Dependency_tracer::add(const prop::detail::Property_base *pb) {
	if (current_widget.first) {
		auto &widget_properties = widgets[current_widget.first].data[current_widget.second].properties;
		if (not prop::contains(widget_properties, pb)) {
			widget_properties.push_back(pb);
		}
	}
	if (properties.contains(pb)) {
		return;
	}
	properties[pb] = {
#ifdef PROPERTY_NAMES
		.type = pb->type,
#else
		.type = "",
#endif
		.name = "",
		.dependents = {std::begin(pb->dependents), std::end(pb->dependents)},
		.dependencies = {std::begin(pb->implicit_dependencies), std::end(pb->implicit_dependencies)},
		.widget = current_widget.first,
	};
	for (auto deps : pb->dependents) {
		add(deps);
	}
	for (auto deps : pb->implicit_dependencies) {
		add(deps);
	}
}
