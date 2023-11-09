#pragma once

#include "../utility/polywrap.h"
#include "widget.h"

#include <vector>

namespace prop {
	class Vertical_layout : public prop::Widget {
		public:
		void update() override;
		Property<std::vector<prop::Polywrap<prop::Widget>>> children;

		private:
	};
} // namespace prop
