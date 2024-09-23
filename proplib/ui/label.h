#pragma once

#include "proplib/ui/widget.h"
#include "proplib/utility/font.h"
#include "proplib/utility/style.h"

#include <memory>
#include <string>

namespace prop {
	void swap(class Label &lhs, class Label &rhs);
	class Label : public prop::Widget {
		public:
		struct Parameters {
			prop::Property<std::string> text = "";
			prop::Property<prop::Font> font = [] { return prop::default_style.font; };
			prop::Property<int> font_size = [] { return prop::default_style.font_size; };
			prop::Widget::Parameters widget = {};
		};
		Label();
		Label(Parameters &&parameters);
		Label(Label &&other);
		Label &operator=(Label &&other);
		~Label() override;

		void draw(struct Draw_context context) const override;
		friend void swap(Label &lhs, Label &rhs);

		prop::Property<std::string> text;
		prop::Property<prop::Font> font;
		prop::Property<int> font_size;

		private:
		std::unique_ptr<struct Label_privates> privates;
	};
} // namespace prop
