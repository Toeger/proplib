#include "style.h"

//TODO: Load system standard style and monitor changes to it
prop::Style prop::default_style = [] {
	prop::Style retval;
	if (!retval.font.apply([](prop::Font &f) { return f.load("/usr/share/fonts/truetype/ubuntu/Ubuntu-M.ttf"); })) {
		throw std::runtime_error{"TODO: implement proper font finding"};
	}
	retval.font_size = 24;
	retval.hover_time = std::chrono::milliseconds{200};
	return retval;
}();
