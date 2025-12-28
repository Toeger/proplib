#pragma once

#include "prop/ui/widget.h"
#include "prop/utility/font.h"
#include "prop/utility/style.h"

#include <string>

namespace prop {
	void swap(class Label &lhs, class Label &rhs) noexcept;
	class Label : public prop::Widget {
		public:
		struct Parameters {
			prop::Property<std::string> text = "";
			prop::Property<prop::Font> font = [] { return prop::Style::default_style.font; };
			prop::Widget::Parameters widget = {};
		};
		Label();
		Label(Parameters parameters);
		Label(Label &&other) noexcept;
		Label &operator=(Label &&other) noexcept;
		~Label() override;

		void draw(prop::Canvas canvas) const override;
		friend void swap(Label &lhs, Label &rhs) noexcept;

		prop::Property<std::string> text;
		prop::Property<prop::Font> font;
	};
} // namespace prop
