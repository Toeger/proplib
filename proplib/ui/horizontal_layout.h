#pragma once

#include "../utility/font.h"
#include "widget.h"

#include <memory>
#include <string>

namespace prop {
	class Label : public prop::Widget {
		public:
		Label(std::string text = {});
		Label(Label &&other);
		Label &operator=(Label &&other);
		~Label();

		void update() override;
		friend void swap(Label &lhs, Label &rhs);

		prop::Property<std::string> text;
		prop::Property<prop::Font> font;
		std::unique_ptr<struct Label_privates> privates;
	};
} // namespace prop
