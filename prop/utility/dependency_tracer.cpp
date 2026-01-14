#include "dependency_tracer.h"
#include "color.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <regex>

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
				   << prop::Color::static_text << '@' << prop::color_address(member.get_address()) << ' '
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
		bool colored = false;
		const auto end_color = [&colored, &result] {
			if (not colored) {
				return;
			}
			colored = false;
			if (result.back() == '>') {
				while (result.back() != '<') {
					result.pop_back();
				}
				result.pop_back();
			} else {
				result += "</font>";
			}
		};
		for (std::size_t i = 0; i < std::size(sv); i++) {
			const char chr = sv[i];
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
				case '\033': {
					end_color();
					if (sv[++i] == '[') {
						i++;
						std::size_t end_pos = i;
						while (end_pos + 1 < std::size(sv) and sv[++end_pos] != 'm') {
						}
						static const std::regex re(R"((\d+);(\d+);(\d+);(\d+);(\d+))");
						std::cmatch m;
						if (std::regex_match(&sv[i], &sv[end_pos], m, re)) {
							if (m[1].str() == "38" and m[2].str() == "2") {
								result += std::format("<font color='#{:02X}{:02X}{:02X}'>", stoi(m[3].str()),
													  stoi(m[4].str()), stoi(m[5].str()));
								colored = true;
							}
						}
						i = end_pos - 1;
					}
					while (i + 1 < std::size(sv) and sv[++i] != 'm') {
					}
				} break;
				default:
					result += chr;
			}
		}
		end_color();
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
		std::uint32_t rgb[3] = {0x0, 0x0, 0x0};
		for (int i = 0; i < 7; i++) {
			for (int ci = 0; ci < 3; ci++) {
				rgb[ci] |= (value & 1) << i;
				value >>= 1;
			}
		}
		return std::format("{:02x}{:02x}{:02x}", rgb[min_base_index], rgb[(min_base_index + 1) % 3],
						   rgb[(min_base_index + 2) % 3]);
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
		Command _{"bgcolor=\"#303030\""};
		Command _{"overlap=\"false\""};
		Command _{"fontname=\"FiraCode-Medium\""};

		for (auto &[link, data] : object_data) {
			if (data.widget_data.size()) { //is a widget
				auto &widget_link = link;
				Block _{dot_name(widget_link), "[", "];"};
				Command _{"shape=plaintext"};
				Block _{"label =", "<", ">"};
				HTML_tag _{"table",
						   "border='1' cellborder='0' cellspacing='1' bgcolor='#" + color_code(widget_link) + "'"};
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
							HTML_tag{"font", std::format("color='#{:hex}'", prop::Color::type),
									 bold(html_encode(prop::color_type(std::string{base->type})))};
						}
						{
							HTML_tag _{"td"};
							HTML_tag{"font", std::format("color='#{:hex}'", prop::Color::variable_name),
									 bold(html_encode(data.name))};
						}
						HTML_tag _{"td", "", ""};
						{
							HTML_tag _{"td"};
							HTML_tag{"font", std::format("color='#{:hex}'", prop::Color::address),
									 bold(html_encode(prop::color_address(widget_link)))};
						}
					}
					for (auto &member : base->members) {
						{
							HTML_tag _{"tr"};
							{
								HTML_tag _{"td", "align='right'",
										   html_encode(prop::color_type(std::string{member.get_type()}))};
							}
							{
								HTML_tag _{"td", "align='center'"};
								HTML_tag{"font", std::format("color='#{:hex}'", prop::Color::variable_name),
										 html_encode(member.name)};
							}
							const auto value = member.get_value_string();
							const auto value_string =
								value.size() ?
									std::format(
										"<font color='#{:hex}'>{{</font><font color='#{:hex}'>{}</font><font "
										"color='#{:hex}'>}}</font>",
										prop::Color::static_text, prop::Color::variable_value, html_encode(value),
										prop::Color::static_text) :
									std::format("<font color='#{:hex}'>{{}}</font>", prop::Color::static_text);
							HTML_tag _{"td", "align='left'", value_string};
							HTML_tag _{"td",
									   "align='left' port='property_" + prop::to_string(member.get_address()) + "'"};
							HTML_tag{"font", std::format("color='#{:hex}'", prop::Color::address),
									 html_encode(prop::color_address(member.get_address()))};
						}
					}
				}
			} else { //is not a widget
				if (data.parent) {
					continue;
				}
				Block _{dot_name(link), "[", "];"};
				Command _{"shape=rect"};
				const auto value = link->displayed_value();
				const auto value_string =
					value.size() ? std::format(
									   "<font color='#{:hex}'>{{</font><font color='#{:hex}'>{}</font><font "
									   "color='#{:hex}'>}}</font>",
									   prop::Color::static_text, prop::Color::variable_value, html_encode(value),
									   prop::Color::static_text) :
								   std::format("<font color='#{:hex}'>{{}}</font>", prop::Color::static_text);
				Command _{
					std::format("label=<"
								"<font color='#{:hex}'>{}</font>"  //type
								"<font color='#{:hex}'>@</font>"   //@
								"{} "							   //address
								"<font color='#{:hex}'>{}</font> " //name
								"{}"							   //value
								">",
								prop::Color::type, html_encode(prop::color_type(std::string{link->type()})),  //type
								prop::Color::static_text,													  //@
								html_encode(prop::color_address(link)),										  //address
								prop::Color::variable_name, html_encode(data.name.empty() ? " " : data.name), //name
								value_string																  //value
								)};
				Command _{"style=\"filled\""};
				Command _{"fillcolor=\"#" + color_code(link) + "\""};
			}
		}
		for (auto &[link, data] : object_data) {
			for (auto &dep : link->get_dependencies()) {
				if (dep == nullptr) {
					continue;
				}
				Command _{std::format("{} -> {} [style=\"{}\"]",
									  dot_name(dep, object_data.at(dep).parent ? Alignment::right : Alignment::none),
									  dot_name(link, data.parent ? Alignment::right : Alignment::none),
									  dep.is_required() ? "bold" : "dashed")};
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

std::string prop::Dependency_tracer::dot_name(const Property_link *link, prop::Alignment alignment) const {
	assert(alignment <= center);
	if (alignment > center) {
		alignment = none;
	}
	constexpr auto direction = std::to_array({
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
	});
	if (auto pit = object_data.find(link); pit != std::end(object_data)) {
		auto &data = pit->second;
		const std::string parentage =
			data.widget_data.is_empty() and data.parent ? dot_name(data.parent->link, none) + ":" : "";
		const std::string_view type = data.widget_data.size() ? "widget" : "property";
		return std::format("{}{}_{}{}", parentage, type, prop::to_string(link), direction[alignment]);
	}
	return "unknown_" + prop::to_string(link);
}
