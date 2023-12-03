#pragma once

#include "../ui/label.h"
#include "../utility/font.privates.h"

#include <SFML/Graphics/Text.hpp>
#include <cmath>

namespace sf {
	class RenderWindow;
}

namespace prop {
	struct Label_privates {
		Label_privates(Label *p_label)
			: label{p_label}
			, text_sizer{[this] {
				sf::Text sftext;
				sftext.setString(label->text.get());
				sftext.setFont(label->font.get().font_privates.get()->font);
				auto bounds = sftext.getLocalBounds();
				auto width = bounds.width;
				auto height = bounds.height;
				label->preferred_height = std::ceil(bounds.height);
				label->preferred_width = std::ceil(bounds.width);
			}} {}
		Label *label;
		Property<void> text_sizer;
	};
} // namespace prop
