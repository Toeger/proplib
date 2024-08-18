#pragma once

#include "utility/polywrap.h"
#include "widget.h"

#include <memory>
#include <vector>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

namespace prop {
	void swap(class Vertical_layout &lhs, class Vertical_layout &rhs);

	class Vertical_layout : public prop::Widget {
		public:
		Vertical_layout();
		template <class... Children>
		Vertical_layout(Children &&...children)
			requires(std::is_convertible_v<decltype(std::forward<Children>(children)), prop::Polywrap<prop::Widget>> &&
					 ...);
		Vertical_layout(Vertical_layout &&other);
		~Vertical_layout();
		Vertical_layout &operator=(Vertical_layout &&other);
		void draw(struct Draw_context context) const override;
		friend void swap(Vertical_layout &lhs, Vertical_layout &rhs);
#ifdef PROPERTY_NAMES
		void set_name(std::string_view name) override;
		prop::Property<void> name_updater;
		Vertical_layout(std::string_view name);
		template <class... Children>
		Vertical_layout(std::string_view name, Children &&...children)
			requires(std::is_convertible_v<decltype(std::forward<Children>(children)), prop::Polywrap<prop::Widget>> &&
					 ...)
			: Vertical_layout{children...} {
			set_name(name);
		}
#endif

		prop::Property<std::vector<prop::Polywrap<prop::Widget>>> children;
#define PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS PROP_X(children)

		private:
		std::unique_ptr<struct Vertical_layout_privates> privates;
	};
} // namespace prop

template <class... Children>
prop::Vertical_layout::Vertical_layout(Children &&...children)
	requires(std::is_convertible_v<decltype(std::forward<Children>(children)), prop::Polywrap<prop::Widget>> && ...)
	: Vertical_layout{} {
	std::vector<prop::Polywrap<prop::Widget>> children_list;
	children_list.reserve(sizeof...(children));
	(children_list.emplace_back(std::forward<Children>(children)), ...);
	this->children = std::move(children_list);
#ifdef PROPERTY_NAMES
	set_name("Vertical_layout");
#endif
}
