#pragma once

#include "../utility/property.h"

#include <memory>

namespace prop {
	void swap(class Widget &lhs, class Widget &rhs);
	class Widget {
		public:
		Widget();
		Widget(Widget &&other);
		Widget &operator=(Widget &&other);

		virtual ~Widget();
		virtual void draw(struct Draw_context context) const;
		friend void swap(Widget &lhs, Widget &rhs);

		Property<int> x;
		Property<int> y;
		Property<int> width;
		Property<int> height;
		Property<int> preferred_width;
		Property<int> preferred_height;
		Property<bool> visible = true;

		std::unique_ptr<struct Widget_privates> privates;
	};
} // namespace prop
