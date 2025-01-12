#pragma once

namespace prop {
	enum Orientation {
		none,
		left,
		right,
		hcenter = left | right,
		top,
		top_left = top | left,
		top_right = top | right,
		top_center = top | hcenter,
		bottom,
		bottom_left = bottom | left,
		bottom_right = bottom | right,
		bottom_center = bottom | hcenter,
		vcenter = top | bottom,
		center_left = vcenter | left,
		center_right = vcenter | right,
		center = hcenter | vcenter,
	};
}
