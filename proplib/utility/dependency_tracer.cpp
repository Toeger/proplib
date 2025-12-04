#include "dependency_tracer.h"
#include "color.h"

#include <algorithm>
#include <format>
#include <fstream>

void prop::Dependency_tracer::print_widget_trace(std::ostream &os) const {
	for (auto &[address, common_data] : widgets) {
		os << prop::Color::variable_name << common_data.name << prop::Color::static_text << '@' << prop::Color::address
		   << address << '\n';
		for (auto &base : common_data.bases) {
			os << prop::Color::static_text << "as " << prop::Color::type << base.type << prop::Color::static_text
			   << ":\n";
			for (auto &property : base.properties) {
				auto &property_data = properties.find(property)->second;
				os << '\t' << prop::Color::type << property_data.type << ' ' << prop::Color::variable_name
				   << property_data.name << prop::Color::static_text << '@' << prop::Color::address << property << "\n";
			}
		}
	}
	os << prop::Color::reset;
}

std::intptr_t prop::Dependency_tracer::global_base_address = reinterpret_cast<std::intptr_t>(&global_base_address);
std::intptr_t prop::Dependency_tracer::stack_base_address = [] {
	int i;
	return reinterpret_cast<std::intptr_t>(std::launder(&i));
}();
std::intptr_t prop::Dependency_tracer::heap_base_address =
	reinterpret_cast<std::intptr_t>(std::launder(std::make_unique<int>().get()));

namespace Dot {
	static std::string html_encode(std::string_view sv) {
		std::string result;
		for (auto &chr : sv) {
			switch (chr) {
				case '<':
					result += "&lt;";
					break;
				case '>':
					result += "&gt;";
					break;
				case '"':
					result += "&quot;";
					break;
				case '&':
					result += "&amp;";
					break;
				default:
					result += chr;
			}
		}
		return result;
	}

	static std::string dot_encode(std::string_view sv) {
		std::string result;
		for (auto &chr : sv) {
			switch (chr) {
				case '<':
					result += "_";
					break;
				case '>':
					result += "_";
					break;
				case '"':
					result += "_";
					break;
				case '&':
					result += "_";
					break;
				case ':':
					result += "_";
					break;
				default:
					result += chr;
			}
		}
		return result;
	}

	static constexpr std::string_view indent_with = "  ";
	static std::string indent;
	static std::ostream *target;

	static void increment_indent() {
		indent += indent_with;
	}
	static void decrement_indent() {
		for (std::size_t i = 0; i < indent_with.size(); i++) {
			indent.pop_back();
		}
	}

	struct Indent {
		Indent() {
			increment_indent();
		}
		Indent(const Indent &) {
			increment_indent();
		}
		~Indent() {
			decrement_indent();
		}
	};

	struct Init_print {
		Init_print(std::string_view sv) {
			*target << sv;
		}
	};

	struct Exit_print {
		std::string str;
		Exit_print(std::string s = "")
			: str{std::move(s)} {}
		~Exit_print() {
			*target << str;
		}
	};

	struct HTML_tag {
		HTML_tag(std::string tag)
			: init{indent + "<" + tag + ">\n"}
			, exit{indent + "</" + tag + ">\n"}
			, indention{Indent{}} {}
		HTML_tag(std::string tag, std::string attributes)
			: init{indent + '<' + tag + ' ' + attributes + ">\n"}
			, exit{indent + "</" + tag + ">\n"}
			, indention{Indent{}} {}
		HTML_tag(std::string tag, std::string attributes, std::string data)
			: init{indent + '<' + tag + (attributes.size() ? " " : "") + attributes + ">" + data + "</" + tag + ">\n"} {
		}
		Init_print init;
		Exit_print exit;
		std::optional<Indent> indention;
	};

	struct Command {
		Command(std::string_view command) {
			*target << indent << command << ";\n";
		}
	};

	struct Block {
		Block()
			: init{indent + "{\n"}
			, exit{indent + "}\n"} {}
		Block(std::string command, std::string start = "{", std::string stop = "}")
			: init{indent + std::move(command) + " " + start + "\n"}
			, exit{indent + stop + "\n"} {}
		Init_print init;
		Exit_print exit;
		Indent indentation;
	};

	static std::string bold(std::string_view sv) {
		return "<b>" + std::string{sv} + "</b>";
	}

	static std::string color_code(const void *address) {
		const std::array bases = {
			prop::Dependency_tracer::global_base_address,
			prop::Dependency_tracer::stack_base_address,
			prop::Dependency_tracer::heap_base_address,
		};
		auto min_base_index = std::min_element(std::begin(bases), std::end(bases),
											   [&](std::intptr_t l, std::intptr_t r) {
												   return std::abs(l - reinterpret_cast<std::intptr_t>(address)) <
														  std::abs(r - reinterpret_cast<std::intptr_t>(address));
											   }) -
							  std::begin(bases);

		std::intptr_t value = reinterpret_cast<std::intptr_t>(address) - bases[min_base_index];
		while (value > 0x1fffff) {
			value >>= 1;
		}
		std::uint32_t rgb[3] = {0x80, 0x80, 0x80};
		for (int i = 0; i < 7; i++) {
			for (int ci = 0; ci < 3; ci++) {
				rgb[ci] |= (value & 1) << i;
				value >>= 1;
			}
		}
		value = rgb[min_base_index] << 16 | rgb[(min_base_index + 1) % 3] << 8 | rgb[(min_base_index + 2) % 3];
		return (std::stringstream{} << std::hex << value).str();
	}
} // namespace Dot

void prop::Dependency_tracer::to_image(std::filesystem::path output_path) const {
	using namespace Dot;
	const auto extension = output_path.extension().string().erase(0, 1);
	output_path.replace_extension(".dot");
	std::ofstream file{output_path};
	{
		target = &file;
		Block _{"digraph G"};
		Command _{"overlap=\"false\""};

		//define widget nodes
		for (auto &[address, common_data] : widgets) {
			Block _{dot_name(address), "[", "];"};
			Command _{"shape=plaintext"};
			Block _{"label =", "<", ">"};
			HTML_tag _{"table", "border='1' cellborder='0' cellspacing='1' bgcolor='#" + color_code(address) + "'"};
			{
				HTML_tag _{"tr"};
				HTML_tag _{"td", "", ""};
				{
					HTML_tag _{"td", ""};
					HTML_tag{"font", "color='darkgreen'", bold(html_encode(common_data.name))};
				}
				HTML_tag _{"td", "", ""};
				HTML_tag _{"td", ""};
				HTML_tag{"font", "color='blue'", prop::to_string(address)};
			}
			{
				HTML_tag _{"tr"};
				HTML_tag _{"td", "", bold("Type")};
				HTML_tag _{"td", "", bold("Name")};
				HTML_tag _{"td", "", bold("Value")};
				HTML_tag _{"td", "", bold("Address")};
			}

			for (auto &base : std::views::reverse(common_data.bases)) {
				{
					HTML_tag _{"tr"};
					{
						HTML_tag _{"td", "align='right'"};
						HTML_tag{"font", "color='darkred'", html_encode(base.type)};
					}
					HTML_tag _{"td", "", ""};
					HTML_tag _{"td", "", ""};
					HTML_tag _{"td", "", ""};
				}
				for (auto &property : base.properties) {
					auto &property_data = properties.find(property)->second;
					auto name = property_data.name == "" ? "<unnamed>" : property_data.name;
					{
						HTML_tag _{"tr"};
						{
							HTML_tag _{"td", "align='right'"};
							HTML_tag{"font", "color='darkred'", html_encode(property_data.type)};
						}
						{
							HTML_tag _{"td", "align='center'"};
							HTML_tag{"font", "color='darkgreen'", html_encode(name)};
						}
						HTML_tag _{"td", "align='left'", html_encode(property_data.value)};
						HTML_tag _{"td", "align='left' port='property_" + prop::to_string(property) + "'"};
						HTML_tag{"font", "color='blue'", prop::to_string(property)};
					}
				}
			}
		}

		//define non-widget property nodes
		for (auto &[address, data] : properties) {
			if (data.widget) {
				continue;
			}
			Block _{dot_property_name(address), "[", "];"};
			Command _{"shape=rect"};
			Command _{
				std::format("label=<<font color='darkred'>{}</font> <font color='darkgreen'>{}</font> {} <font "
							"color='blue'>{}</font>>",
							html_encode(data.type), data.name.empty() ? "<unnamed>" : html_encode(data.name),
							html_encode(data.value), prop::to_string(address))};
			Command _{"style=\"filled\""};
			Command _{"fillcolor=\"#" + color_code(address) + "\""};
		}

		//arrows between widgets
		for (auto &[address, common_data] : widgets) {
			for (auto &base : std::views::reverse(common_data.bases)) {
				for (auto &child : base.children) {
					Command _{
						std::format("{} -> {} [style=\"bold\" color=\"purple\"]", dot_name(address), dot_name(child))};
				}
			}
		}

		//arrows between properties
		for (auto &[address, data] : properties) {
			//dependents? Dependencies?
			for (auto &dependent : data.dependents) {
				if (auto pit = properties.find(address); pit != std::end(properties) && pit->second.widget) {
					if (auto dit = properties.find(dependent);
						dit != std::end(properties) && dit->second.widget == pit->second.widget) {
						//same widget
						Command _{dot_property_name(address) + " -> " + dot_property_name(dependent) +
								  " [color=\"red\" style=\"bold\"]"};
						continue;
						static int i = 1;
						const auto color = i == 1 ? "red" : "red";
						if (i == 1) {
							Command _{"dummy [shape=circle,width=.01,height=.01,label=\"\"]"};
							Command _{std::format("{} -> {} [color=\"{}\" style=\"bold\" arrowhead=none]",
												  dot_property_name(address), "dummy", color)};
						}
						Command _{std::format("{} -> {} [color=\"{}\" style=\"bold\"]", "dummy",
											  dot_property_name(dependent), color)};
						Command _{std::format("{{rank=same {} {}}}", "dummy", dot_property_name(address))};
						i++;
						continue;
					}
				}
				Command _{dot_property_name(address) + " -> " + dot_property_name(dependent)};
			}
		}
	}
	file.close();
	const auto file_path = output_path.string();
	auto command = "dot -T" + extension + " \"" + output_path.string() + "\"";
	output_path.replace_extension("." + extension);
	command += " -o \"" + output_path.string() + "\"";
	std::system(command.c_str());
}

void prop::Dependency_tracer::add(const prop::Property_link *pb) {
	if (pb == nullptr) {
		return;
	}
	if (current_sub_widget) {
		auto &ps = current_sub_widget->properties;
		if (auto it = std::lower_bound(std::begin(ps), std::end(ps), pb); it == std::end(ps) or *it != pb) {
			ps.insert(it, pb);
			if (properties.contains(pb)) {
				properties[pb].widget = current_widget;
				return;
			}
		}
	}
	if (properties.contains(pb)) {
		return;
	}
	auto &&dependents = pb->get_dependents();
	auto &&dependencies = pb->get_dependencies();
	properties[pb] = {
		.type = pb->type(),
		.name = pb->custom_name,
		.value = pb->displayed_value(),
		.dependents = {std::begin(dependents), std::end(dependents)},
		.dependencies = {std::begin(dependencies), std::end(dependencies)},
		.widget = current_widget,
	};
	Make_current _{nullptr, *this};
	for (auto &deps : pb->dependencies) {
		add(deps);
	}
}

std::string prop::Dependency_tracer::dot_property_name(const void *p, prop::Alignment alignment) const {
	if (alignment > center) {
		alignment = none;
	}
	const char *direction[] = {
		"",	   //none,
		":w",  //left,
		":e",  //right,
		":c",  //horizontal_center,
		":n",  //top,
		":nw", //top_left,
		":ne", //top_right,
		":n",  //top_center,
		":s",  //bottom,
		":sw", //bottom_left,
		":se", //bottom_right,
		":c",  //bottom_center,
		":c",  //vertical_center,
		":w",  //center_left,
		":e",  //center_right,
		":c",  //center,
	};
	if (auto pit = properties.find(p); pit != std::end(properties)) {
		if (pit->second.widget) {
			return std::format("widget_{}:property_{}{}", prop::to_string(pit->second.widget), prop::to_string(p),
							   direction[alignment]);
		}
		return "property_" + prop::to_string(p);
	}
	return "unknown_" + prop::to_string(p);
}
