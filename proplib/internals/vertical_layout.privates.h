#include "proplib/ui/vertical_layout.h"
#include "proplib/utility/property.h"

#include <algorithm>

namespace prop {
	struct Vertical_layout_privates {
		Vertical_layout_privates(prop::Vertical_layout *layout_)
			: layout{layout_}
			, layout_updater{[this] {
				layout->children.apply([this](std::vector<prop::Polywrap<prop::Widget>> &children) {
					int pref_width = 0;
					int pref_height = 0;
					int y = layout->y;
					for (auto &child : children) {
						pref_width = std::max(child->preferred_width.get(), pref_width);
						pref_height += child->preferred_height;
						child->x = layout->x;
						child->y = y;
						y += child->preferred_height; //+ layout->margin
					}
					layout->preferred_width = pref_width;
					layout->preferred_height = pref_height;
					for (auto &child : children) {
						child->width = layout->width;
						child->height = child->preferred_height;
					}
				});
			}} {}
		prop::Vertical_layout *layout;
		Property<void> layout_updater;
	};
} // namespace prop
