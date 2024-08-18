#pragma once

#include "ui/button.h"
#include "utility/font.privates.h"

#include <SFML/Graphics/Text.hpp>
#include <cmath>

namespace prop {
	struct Button_privates {
		Button_privates(Button *p_button)
			: button{p_button}
			, text_sizer{[this] {
				sf::Text sftext;
				sftext.setString(button->text.get());
				sftext.setFont(button->font.get().font_privates.get()->font);
				sftext.setCharacterSize(button->font_size);
				button->preferred_height = button->font_size + 4;
				button->preferred_width = std::ceil(sftext.getLocalBounds().width) + 4;
			}} {}
		Button *button;
		Property<void> text_sizer;
	};
} // namespace prop
