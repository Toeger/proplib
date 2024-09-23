#pragma once

#include "proplib/utility/property.h"
#include "proplib/utility/signal.h"

#include <memory>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

namespace prop {
	void swap(class Widget &lhs, class Widget &rhs);
	class Widget {
		public:
		struct Parameters {
			Property<int> x = 0;
			Property<int> y = 0;
			Property<int> width = 0;
			Property<int> height = 0;
			Property<int> preferred_width = 0;
			Property<int> preferred_height = 0;
			Property<bool> visible = true;
		};
		Widget();
		Widget(Parameters &&);
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

		Signal<int, int> left_clicked;
		Signal<int, int> right_clicked;
		Signal<int, int> middle_clicked;

		private:
		std::unique_ptr<struct Widget_privates> privates;
	};
} // namespace prop
