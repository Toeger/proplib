#pragma once

#include "../utility/font.h"
#include "widget.h"

#include <memory>
#include <string>

namespace prop {
	void swap(class Label &lhs, class Label &rhs);
	class Label : public prop::Widget {
		public:
		Label(std::string text = {});
		Label(Label &&other);
		Label &operator=(Label &&other);
		~Label();

		void draw(struct Draw_context context) const override;
		friend void swap(Label &lhs, Label &rhs);

		prop::Property<std::string> text;
		prop::Property<prop::Font> font;
		prop::Property<int> font_size;

		private:
		std::unique_ptr<struct Label_privates> privates;
	};
} // namespace prop
