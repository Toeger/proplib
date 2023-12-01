#pragma once

#include "../utility/property.h"

#include <memory>

namespace prop {
	class Widget;
	void swap(prop::Widget &lhs, prop::Widget &rhs);
	class Label;
	class Widget {
		public:
		Widget();
		Widget(Widget &&other);
		Widget &operator=(Widget &&other);

		virtual ~Widget();
		virtual void update();
		friend void swap(Widget &lhs, Widget &rhs);

		Property<int> x;
		Property<int> y;
		Property<int> width;
		Property<int> height;
		Property<int> preferred_width;
		Property<int> preferred_height;
		std::unique_ptr<struct Widget_privates> privates;
	};
} // namespace prop
