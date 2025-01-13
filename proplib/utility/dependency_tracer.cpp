#include "dependency_tracer.h"
#include "color.h"

static std::ostream &operator<<(std::ostream &os, prop::Color c) {
	return os << prop::Console_text_color{c};
}

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
	os << prop::console_reset_text_color;
}
