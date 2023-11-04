#pragma once

#include "../utility/property.h"

#include <memory>

namespace prop {
	class Widget {
		public:
		Widget();
		virtual ~Widget();
		virtual void update();

		Property<int> x;
		Property<int> y;
		Property<int> width;
		Property<int> height;

		std::unique_ptr<struct Widget_privates> privates;
	};
} // namespace prop
