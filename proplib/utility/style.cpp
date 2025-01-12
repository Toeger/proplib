#include "style.h"

//TODO: Load system standard style and monitor changes to it
prop::Style prop::Style::default_style = {
	.font = prop::Font{{
		.color = prop::Color::black,
		.name = "/usr/share/fonts/truetype/ubuntu/Ubuntu-M.ttf",
		.orientation = prop::Orientation::top_left,
		.pixel_size = 12,
		.bold = 1.f,
		.italic = false,
		.strikeout = false,
		.subscript = false,
		.superscript = false,
		.underline = false,
	}},
	.hover_time = std::chrono::milliseconds{200},
};
