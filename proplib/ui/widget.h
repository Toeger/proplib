#pragma once

#include "utility/property.h"
#include "utility/signal.h"

#include <memory>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

namespace prop {
	void swap(class Widget &lhs, class Widget &rhs);
	class Widget {
		public:
		Widget();
		Widget(Widget &&other);
		Widget &operator=(Widget &&other);

		virtual ~Widget();
		virtual void draw(struct Draw_context context) const;
#ifdef PROPERTY_NAMES
		virtual void set_name(std::string_view name);
		Widget(std::string_view name);
#endif
		friend void swap(Widget &lhs, Widget &rhs);

		Property<int> x;
		Property<int> y;
		Property<int> width;
		Property<int> height;
		Property<int> preferred_width;
		Property<int> preferred_height;
		Property<bool> visible = true;
#define PROP_WIDGET_PROPERTY_MEMBERS                                                                                   \
	PROP_X(x) PROP_X(y) PROP_X(width) PROP_X(height) PROP_X(preferred_width) PROP_X(preferred_height) PROP_X(visible)

		Signal<int, int> left_clicked;
		Signal<int, int> right_clicked;
		Signal<int, int> middle_clicked;

		std::unique_ptr<struct Widget_privates> privates;
	};
} // namespace prop
