#pragma once

#include "../utility/binding.h"
#include "../utility/font.privates.h"
#include "label.h"

#include <SFML/Graphics/Text.hpp>
#include <cmath>

namespace sf {
	class RenderWindow;
}

namespace prop {
	struct Label_privates {
		Label_privates(Label *p_label)
			: label{p_label} {
			text_sizer.bind<std::string>(
				[this](const prop::Property<std::string> *text) {
					sf::Text sftext;
					sftext.setString(text->get());
					sftext.setFont(label->font.get().font_privates.get()->font);
					const auto &bounds = sftext.getLocalBounds();
					label->preferred_height = std::ceil(bounds.height);
					label->preferred_width = std::ceil(bounds.width);
				},
				&label->text);
		}
		Label *label;
		Binding text_sizer;
	};
} // namespace prop
