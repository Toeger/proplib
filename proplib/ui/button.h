#pragma once

#include "proplib/ui/widget.h"
#include "proplib/utility/font.h"
#include "proplib/utility/style.h"

#include <functional>

namespace prop {
	class Button;
	void swap(Button &lhs, Button &rhs) noexcept;

	class Button : public prop::Widget {
		struct Parameters {
			prop::Property<std::string> text = "";
			prop::Property<prop::Font> font = [] { return prop::Style::default_style.font; };
			std::function<void()> callback = [] {};
			prop::Widget::Parameters widget = {};
		};

		public:
		Button();
		Button(Parameters &&parameters);
		Button(Button &&other) noexcept;
		~Button() override;
		Button &operator=(Button &&other) noexcept;

		void draw(prop::Canvas context) const override;
		void trace(Dependency_tracer &dependency_tracer) const override;
		friend void swap(Button &lhs, Button &rhs) noexcept;

		prop::Property<std::string> text;
		prop::Property<prop::Font> font;
		std::function<void()> callback;
	};
} // namespace prop
