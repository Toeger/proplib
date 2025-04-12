#pragma once

namespace prop {
	enum Alignment : unsigned char {
		none,
		left,
		right,
		horizontal_center = left | right,
		top,
		top_left = top | left,
		top_right = top | right,
		top_center = top | horizontal_center,
		bottom,
		bottom_left = bottom | left,
		bottom_right = bottom | right,
		bottom_center = bottom | horizontal_center,
		vertical_center = top | bottom,
		center_left = vertical_center | left,
		center_right = vertical_center | right,
		center = horizontal_center | vertical_center,
	};
}
