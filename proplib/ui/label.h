#pragma once

#include "../utility/font.h"
#include "widget.h"

#include <memory>
#include <string>

namespace prop {
	class Label : public prop::Widget {
		public:
		Label(std::string text = {});
		~Label();
		prop::Property<std::string> text;
		prop::Property<prop::Font> font;

		void update() override;

		std::unique_ptr<struct Label_privates> privates;
	};
} // namespace prop
