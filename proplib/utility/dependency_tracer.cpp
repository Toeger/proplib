#include "dependency_tracer.h"
#include "color.h"

#include <algorithm>
#include <format>
#include <fstream>

std::string prop::Dependency_tracer::to_string() const {
	std::stringstream ss;
	for (auto &[prop_link, object] : object_data) {
		if (object.parent) {
			continue;
		}
		ss << prop::Color::variable_name << object.name << prop::Color::static_text << '@' << prop::Color::address
		   << prop_link << ' ' << prop::Color::variable_value << prop_link->value_string() << '\n';
		for (auto &data : object.widget_data) {
			ss << prop::Color::static_text << "as " << prop::Color::type << data->type << prop::Color::static_text
			   << ":\n";
			for (auto &member : data->members) {
				struct Name_type_address {
					std::string_view type;
					std::string value;
					const void *address;
				};
				ss << '\t' << prop::Color::type << member.get_type() << ' ' << prop::Color::variable_name << member.name
				   << prop::Color::static_text << '@' << prop::Color::address << member.get_address() << ' '
				   << prop::Color::variable_value << member.get_value_string() << "\n";
			}
		}
	}
	ss << prop::Color::reset;
	return std::move(ss).str();
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
	if (std::filesystem::is_directory(output_path)) {
		output_path /= "tmp.png";
	}
	const auto extension = output_path.extension().string().erase(0, 1);
	output_path.replace_extension(".dot");
	std::ofstream file{output_path};
	{
		target = &file;
		Block _{"digraph G"};
		Command _{"overlap=\"false\""};

		for (auto &[link, data] : object_data) {
			if (data.parent) {
				continue;
			}
			if (data.widget_data.size()) {
				Block _{dot_name(link), "[", "];"};
				Command _{"shape=plaintext"};
				Block _{"label =", "<", ">"};
				HTML_tag _{"table", "border='1' cellborder='0' cellspacing='1' bgcolor='#" + color_code(link) + "'"};
				{
					HTML_tag _{"tr"};
					HTML_tag _{"td", "", ""};
					{
						HTML_tag _{"td", ""};
						HTML_tag{"font", "color='darkgreen'", bold(html_encode(data.name))};
					}
					HTML_tag _{"td", "", ""};
					HTML_tag _{"td", ""};
					HTML_tag{"font", "color='blue'", prop::to_string(link)};
				}
				{
					HTML_tag _{"tr"};
					HTML_tag _{"td", "", bold("Type")};
					HTML_tag _{"td", "", bold("Name")};
					HTML_tag _{"td", "", bold("Value")};
					HTML_tag _{"td", "", bold("Address")};
				}

				for (auto &base : data.widget_data) {
					{
						HTML_tag _{"tr"};
						{
							HTML_tag _{"td", "align='right'"};
							HTML_tag{"font", "color='darkred'", bold(html_encode(base->type))};
						}
						HTML_tag _{"td", "", ""};
						HTML_tag _{"td", "", ""};
						HTML_tag _{"td", "", ""};
					}
					for (auto &member : base->members) {
						{
							HTML_tag _{"tr"};
							{
								HTML_tag _{"td", "align='right'"};
								HTML_tag{"font", "color='darkred'", html_encode(member.get_type())};
							}
							{
								HTML_tag _{"td", "align='center'"};
								HTML_tag{"font", "color='darkgreen'", html_encode(member.name)};
							}
							HTML_tag _{"td", "align='left'", html_encode(member.get_value_string())};
							HTML_tag _{"td",
									   "align='left' port='property_" + prop::to_string(member.get_address()) + "'"};
							HTML_tag{"font", "color='blue'", prop::to_string(member.get_address())};
						}
					}
				}
			} else {
				Block _{dot_name(link), "[", "];"};
				Command _{"shape=rect"};
				Command _{
					std::format("label=<<font color='darkred'>{}</font> <font color='darkgreen'>{}</font> {} <font "
								"color='blue'>{}</font>>",
								html_encode(link->type()), html_encode(data.name.empty() ? "<unnamed>" : data.name),
								html_encode(link->displayed_value()), prop::to_string(link))};
				Command _{"style=\"filled\""};
				Command _{"fillcolor=\"#" + color_code(link) + "\""};
			}
			for (auto &dep : link->get_dependencies()) {
				//if (auto pit = properties.find(address); pit != std::end(properties) && pit->second.widget) {
				//	if (auto dit = properties.find(dependent);
				//		dit != std::end(properties) && dit->second.widget == pit->second.widget) {
				//		//same widget
				//		Command _{dot_property_name(address) + " -> " + dot_property_name(dependent) +
				//				  " [color=\"red\" style=\"bold\"]"};
				//		continue;
				//		static int i = 1;
				//		const auto color = i == 1 ? "red" : "red";
				//		if (i == 1) {
				//			Command _{"dummy [shape=circle,width=.01,height=.01,label=\"\"]"};
				//			Command _{std::format("{} -> {} [color=\"{}\" style=\"bold\" arrowhead=none]",
				//								  dot_property_name(address), "dummy", color)};
				//		}
				//		Command _{std::format("{} -> {} [color=\"{}\" style=\"bold\"]", "dummy",
				//							  dot_property_name(dependent), color)};
				//		Command _{std::format("{{rank=same {} {}}}", "dummy", dot_property_name(address))};
				//		i++;
				//		continue;
				//	}
				//}
				Command _{dot_name(dep) + " -> " + dot_name(link) + (dep.is_required() ? "" : "[style=dashed]")};
			}
		}

#if DELETE_LATER
		//arrows between widgets
		for (auto &[address, common_data] : widgets) {
			for (auto &base : std::views::reverse(common_data.bases)) {
				for (auto &child : base.children) {
					Command _{
						std::format("{} -> {} [style=\"bold\" color=\"purple\"]", dot_name(address), dot_name(child))};
				}
			}
		}
#endif
	}
	file.close();
	const auto file_path = output_path.string();
	auto command = "dot -T" + extension + " \"" + output_path.string() + "\"";
	output_path.replace_extension("." + extension);
	command += " -o \"" + output_path.string() + "\"";
	std::system(command.c_str());
}

std::string prop::Dependency_tracer::dot_name(const Property_link *link, prop::Alignment alignment) const {
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
	if (auto pit = object_data.find(link); pit != std::end(object_data)) {
		auto &data = pit->second;
		if (data.parent) {
			return std::format("widget_{}:property_{}{}", prop::to_string(data.parent), prop::to_string(link),
							   direction[alignment]);
		}
		return "property_" + prop::to_string(link);
	}
	return "unknown_" + prop::to_string(link);
}
