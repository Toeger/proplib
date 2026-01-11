#include "utility.h"

std::string prop::color_type(std::string type) {
	{ //Replace "> >" with ">>"
		size_t start_pos = 0;
		while ((start_pos = type.find("> >", start_pos)) != std::string::npos) {
			type.replace(start_pos, 3, ">>");
			start_pos += 2;
		}
	}
	enum class Highlight { highlighted, not_highlighted, unknown };
	auto insert_color = [&type, start = std::size_t{}, //
						 state = Highlight::unknown,   //
						 not_highlighted = std::format("{:ansi}", prop::Color::type),
						 highlighted = std::format("{:ansi}", prop::Color::type_highlight)](
							std::size_t &i, Highlight highlight) mutable {
		if (state == highlight) {
			return;
		}
		if ((state = highlight) == Highlight::unknown) {
			start = i;
			return;
		}
		const auto &color = state == Highlight::highlighted ? highlighted : not_highlighted;
		type.insert(start, color);
		i += color.size();
		start = i;
	};
	std::size_t i = 0;
	int depth = 0;
	for (; i < std::size(type); i++) {
		switch (type[i]) {
			case ',':
			case '(':
				depth++;
				break;
			case ')':
				depth--;
				i++;
				insert_color(i, Highlight::not_highlighted);
				break;
			case '<':
			case '>': {
				if (depth) {
					continue;
				}
				insert_color(i, Highlight::highlighted);
				i++;
				insert_color(i, Highlight::not_highlighted);
				i--;
			} break;
			case ':':
				if (depth) {
					continue;
				}
				insert_color(i, Highlight::not_highlighted);
				i += 2;
				insert_color(i, Highlight::unknown);
				break;
		}
	}
	insert_color(i, Highlight::highlighted);
	return type;
}
