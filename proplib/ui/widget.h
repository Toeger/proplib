#pragma once

#include "proplib/utility/property.h"
#include "proplib/utility/rect.h"
#include "proplib/utility/signal.h"
#include "proplib/utility/utility.h"

#ifdef PROPERTY_NAMES
#include <string_view>
#endif

namespace prop {
	class Canvas;
	class Dependency_tracer;

	void swap(class Widget &lhs, class Widget &rhs);

	class Widget {
		public:
		struct Parameters {
			Property<prop::Rect> position;
			Property<bool> visible = true;
			Property<prop::Size> min_size = prop::Size{0, 0};
			Property<prop::Size> max_size = prop::Size::max;
			Property<prop::Size> preferred_size = prop::Size{0, 0};
		};
		Widget();
		Widget(Parameters &&);
		Widget(Widget &&other) noexcept;
		Widget &operator=(Widget &&other) noexcept;

		virtual ~Widget();
		virtual void draw(prop::Canvas context) const;
#ifdef PROPERTY_NAMES
		virtual void set_name(std::string_view name);
		virtual void trace(Dependency_tracer &dependency_tracer) const;
		Widget(std::string_view name);
#endif
		friend void swap(Widget &lhs, Widget &rhs);

		prop::Property<prop::Self> self;
		template <class Widget>
		prop::Property<Widget *> selfie(this Widget &widget) {
			return {{[](prop::Self self_) { return static_cast<Widget *>(self_.self); }, widget.self}};
		}
		Property<prop::Rect> position;
		Property<bool> visible = true;
		const Property<prop::Size> &get_min_size() const {
			return min_size;
		}
		const Property<prop::Size> &get_max_size() const {
			return max_size;
		}
		const Property<prop::Size> &get_preferred_size() const {
			return preferred_size;
		}

		protected:
		prop::Property<prop::Size> min_size;
		prop::Property<prop::Size> max_size;
		prop::Property<prop::Size> preferred_size;

		prop::Signal<int, int> left_clicked;
		prop::Signal<int, int> right_clicked;
		prop::Signal<int, int> middle_clicked;
	};
} // namespace prop
