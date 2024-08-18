#pragma once

#include "ui/label.h"
#include "utility/font.privates.h"

#include <SFML/Graphics/Text.hpp>
#include <cmath>

namespace prop {
	struct Label_privates {
		Label_privates(Label *p_label)
			: label{p_label}
			, text_sizer{[this] {
				sf::Text sftext;
				sftext.setString(label->text.get());
				sftext.setFont(label->font.get().font_privates.get()->font);
				sftext.setCharacterSize(label->font_size);
				label->preferred_height = label->font_size;
				label->preferred_width = std::ceil(sftext.getLocalBounds().width);
			}} {}
		Label *label;
		Property<void> text_sizer;
	};
} // namespace prop
