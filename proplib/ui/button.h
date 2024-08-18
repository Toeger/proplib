#pragma once

#include "utility/font.h"
#include "widget.h"

#include <functional>
#include <memory>

namespace prop {
	class Button;
	void swap(Button &lhs, Button &rhs);

	class Button : public prop::Widget {
		public:
		Button();
		Button(std::string text, std::function<void()> on_clicked = []{});
		Button(Button &&other);
		~Button();
		Button &operator=(Button &&other);

		void draw(struct Draw_context context) const override;
		friend void swap(Button &lhs, Button &rhs);

		prop::Property<std::string> text;
		prop::Property<prop::Font> font;
		prop::Property<int> font_size;
		std::function<void()> on_clicked;

		private:
		std::unique_ptr<struct Button_privates> privates;
	};
} // namespace prop
