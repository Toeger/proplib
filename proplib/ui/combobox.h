#include "proplib/ui/widget.h"
#include "proplib/utility/polywrap.h"
#include "proplib/utility/property.h"

#include <string>

namespace prop {
	void swap(class Combobox &lhs, class Combobox &rhs) noexcept;
	class Combobox : public prop::Widget {
		public:
		prop::Property<std::vector<prop::Polywrap<prop::Widget>>> elements;
		prop::Property<int> current_element;
		prop::Property<bool> is_open;

		static prop::Property<std::vector<prop::Polywrap<prop::Widget>>>
		make_entry_representation(const prop::Property<std::vector<std::string>> &source);
	};
} // namespace prop
